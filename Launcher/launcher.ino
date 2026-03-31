#include <Arduino.h>
#include "pin_config.h"
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include <lvgl.h>
#include <SPI.h>
#include <Update.h>
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "XPowersLib.h"
#include "SensorPCF85063.hpp"
#include <esp_sleep.h>

#include "HWCDC.h"

#include "SDManager.h"
#include "ScreenManager.h"
#include "HomeScreen.h"
#include "AppState.h"
#include "AlarmTriggerScreen.h"
#include "lv_fs_sd.h"

#include "HomeNavigation.h"
#include "AudioManager.h"
extern "C" {
  #include "driver/i2s_std.h"
}
#include "ESP_I2S.h"
I2SClass i2s;

#include "esp_check.h"
#include "es8311.h"
#include "canon.h"

#include "AlarmManager.h"

#define EXAMPLE_SAMPLE_RATE 16000
#define EXAMPLE_VOICE_VOLUME 80                  // 0 - 100
#define EXAMPLE_MIC_GAIN (es8311_mic_gain_t)(3)  // 0 - 7
#define EXAMPLE_RECV_BUF_SIZE (10000)

// Buffer di disegno LVGLgvbff 
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 10];

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1,
  LCD_SDIO2, LCD_SDIO3);

Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// Inizializzazione touch
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
std::unique_ptr<Arduino_FT3x68> FT3168(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS));

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  //USBSerial.printf(buf);
  Serial.flush();
}
#endif

HWCDC USBSerial;
SDManager sdManager;
ScreenManager screenManager;
XPowersPMU power;
SensorPCF85063 rtc;
bool isAlwaysOn = false;
bool enableAlwaysOn = false;
bool backHome = false;
unsigned long bootPressedTime = 0;
unsigned long touchLastTime = 0;
int brightness = 120;
HomeNavigation home;
SemaphoreHandle_t g_sdMutex;
bool isCharging = false;
bool alarmSet = false;
unsigned long alarmSetTime = 0;
bool appClose = false;

#define LVGL_TICK_PERIOD_MS 2

void lv_tick_task(void *arg) {
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/* --- Display flush callback --- */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
  int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);

  
  bool touched = fingers > 0;

  if (touched) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }

}
uint32_t lastTriggeredId = 0;
void checkAlarms() {
  time_t now = time(nullptr);

  uint32_t idx = AlarmManager::checkAlarms(now);
  if (idx != 0) {
      const Alarm* a = AlarmManager::getById(idx);
      if (a && a->id != lastTriggeredId) {
          if(isAlwaysOn) exitAlwaysOn();

          lastTriggeredId = a->id;
          ScreenManager::get().openModal(new AlarmTriggerScreen(a->id));
      }
  }
}


void alwaysOn() {
  //gfx->fillScreen(BLACK);

  gfx->setTextColor(0x7BEF);

  // Mostra percentuale batteria
   gfx->fillRect(100, 100, 200, 60, BLACK);
  gfx->setTextSize(5);
  gfx->setCursor(100, 100);
  gfx->println(String(power.getBatteryPercent()) + "%");


  RTC_DateTime datetime = rtc.getDateTime();
  gfx->fillRect(40, 400, 200, 60, BLACK);
  gfx->setTextSize(5);
  gfx->setCursor(40, 400);
  gfx->printf("%02d:%02d",
              datetime.hour,
              datetime.minute);

  // Se è in carica
  if (power.isCharging()) {
    gfx->fillRect(100, 200, 200, 60, BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(100, 200);
    gfx->println("Charging");
  }
}

void exitAlwaysOn() {
  //Wire.begin(IIC_SDA, IIC_SCL); // riattiva touch
  setCpuFrequencyMhz(240);
  isAlwaysOn = false;
  gfx->Display_Brightness(brightness);
  delay(1000);
  // Forza LVGL a ridisegnare lo screen principale
  //lv_obj_t *scr = lv_scr_act();
  //lv_obj_clean(scr);      // cancella vecchi oggetti
  home.open();
  //screenManager.changeScreen(new HomeScreen());
  lv_timer_handler();
  //AudioManager::ampOn();
}

void setup() {
  // put your setup code here, to run once:
  USBSerial.begin(115200);
  pinMode(0, INPUT_PULLUP);

  delay(1000);

  //USBSerial.println("Startup!");

  if (!i2s.begin(I2S_MODE_STD, EXAMPLE_SAMPLE_RATE,
               I2S_DATA_BIT_WIDTH_16BIT,
               I2S_SLOT_MODE_STEREO,
               I2S_STD_SLOT_BOTH)) {
    //USBSerial.println("Failed to initialize I2S bus!");
    return;
  }

  Wire.begin(IIC_SDA, IIC_SCL);

  //AudioManager::init();

  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(brightness);

  // Inizializza PMU
  if (!power.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    //USB////USBSerial.println("Power management init failed!");
  } else {
    //USB////USBSerial.println("Power management ready");
    isCharging = power.isCharging();
  }
  
  if (!rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    //USB//USBSerial.println("Failed to find PCF8563 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  //AudioManager::init();

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  lv_init();
  lv_fs_sd_init();

  g_sdMutex = xSemaphoreCreateMutex();
  if(!sdManager.init()) {
    //USBSerial.println("Cannot Mound Card");
  } else {
    //USBSerial.println("Card Mounted");
  }

  #if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
  #endif


  // Buffer
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_WIDTH * LCD_HEIGHT / 10);

  // Display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Input driver (touch)
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Crea un timer che chiama lv_tick_inc periodicamente
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &lv_tick_task,
    .name = "lv_tick"
  };

  esp_timer_handle_t periodic_timer;
  esp_timer_create(&periodic_timer_args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, LVGL_TICK_PERIOD_MS * 1000);

  AlarmManager::load();
  AlarmManager::initPreferences();
  //ScreenManager::get().changeScreen(new HomeScreen());
  home.open();
}

void loop() {
  lv_timer_handler();
  ScreenManager screenManager = ScreenManager::get();

  checkAlarms();
  if(power.isCharging() != isCharging) {
    isCharging = power.isCharging();
  }

  if(appClose) {
    appClose = false;
    handleAppClose();
  }

  if(touchLastTime != 0 && screenManager.getCurrent()->getId() != APP_HOME) {
    touchLastTime = 0;
  } 
  if(enableAlwaysOn || (touchLastTime != 0 && millis() - touchLastTime > (60000 * 5))) {
    enableAlwaysOn = false;
    touchLastTime = 0;
    startAlwaysOn();

    return;
  }
  
  RTC_DateTime datetime = rtc.getDateTime();
  if(isAlwaysOn) {
    alwaysOn();
    
    int seconds = datetime.second;
    int sleepTime = 60 - seconds;
    esp_sleep_enable_timer_wakeup(sleepTime * 1000000ULL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)0, 0);
    esp_light_sleep_start();
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    

    if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
      exitAlwaysOn();

      return;
    } 
    return;
  }
  if(backHome) {
      backHome = false;
      home.open();
      return;
      //lv_timer_handler();
  }
  if(digitalRead(0) == LOW && bootPressedTime == 0) {
    bootPressedTime = millis();
  }
  if(digitalRead(0) == HIGH && bootPressedTime != 0) {
    if(millis() - bootPressedTime < 1000) {
      handleAppClose();
    } else {
      enableAlwaysOn = true;
    }
    bootPressedTime = 0;

    return;
  }
  screenManager.loop();
  
  int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
  int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if(fingers > 0) {
    // actually disabled due to a bad feedback sound
    // AudioManager::playTap();
    touchLastTime = millis();
    AudioManager::stopAlarm();
    AudioManager::deinit();
  } else if(touchLastTime == 0 && screenManager.getCurrent()->getId() == APP_HOME) {
    touchLastTime = millis();
  }

  screenManager.touch(x, y, fingers);
  //i2s.write((uint8_t *)canon_pcm, canon_pcm_len);
}

void startAlwaysOn() {
  RTC_DateTime datetime = rtc.getDateTime();
  gfx->fillScreen(BLACK);
  isAlwaysOn = true;
  gfx->Display_Brightness(20);
  //Wire.end();  // spegne I2C touch
  //AudioManager::ampOff();struct tm timeinfo;

  // Array per mesi e giorni in italiano
  const char* mesi[] = {
    "Gennaio", "Febbraio", "Marzo", "Aprile", "Maggio", "Giugno",
    "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "Dicembre"
  };

  const char* giorni[] = {
    "Domenica", "Lunedi", "Martedi", "Mercoledi",
    "Giovedi", "Venerdi", "Sabato"
  };

  // Campi RTC
  int giorno = datetime.day;
  int mese = datetime.month - 1;   // ⚠️ RTC = 1-12 → array = 0-11
  int anno = datetime.year;

  // Costruisci struct tm con i dati RTC
  struct tm t;
  t.tm_year = anno - 1900;
  t.tm_mon  = mese;
  t.tm_mday = giorno;
  t.tm_hour = datetime.hour;
  t.tm_min  = datetime.minute;
  t.tm_sec  = datetime.second;

  // Calcola il weekday e timestamp locale
  time_t ts = mktime(&t);  // calcola tm_wday

  struct tm localTime;
  localtime_r(&ts, &localTime);  // applica fuso orario del sistema

  int weekday = localTime.tm_wday; // giorno della settimana corretto

  // Prima riga: Mese Anno
  gfx->setTextSize(3);
  gfx->setCursor(40, 300);
  gfx->printf("%s %d", mesi[mese], anno);

  // Seconda riga: giorno, nome giorno
  gfx->setCursor(40, 340);
  gfx->printf("%d, %s", giorno, giorni[weekday]);

  // Terza riga: ora locale
  gfx->fillRect(40, 400, 200, 60, BLACK);
  gfx->setTextSize(5);
  gfx->setCursor(40, 400);
  gfx->printf("%02d:%02d", localTime.tm_hour, localTime.tm_min);

  setCpuFrequencyMhz(80);
}

void handleAppClose() {
  ScreenManager screenManager = ScreenManager::get();
  if(screenManager.getCurrent() != nullptr) {
    if(screenManager.getCurrent()->getModal()) {
      screenManager.getCurrent()->closeModal();
    } else if(screenManager.getCurrent()->getId() != APP_HOME) {
      home.open();
    } 
  }
}

void returnToLauncher() {
    // Ottieni la partizione di boot attiva (launcher)
    const esp_partition_t* launcherPartition = esp_ota_get_next_update_partition(nullptr);

    if (!launcherPartition) {
        //USBSerial.println("Errore: impossibile trovare la partizione del launcher!");
        return;
    }

    // Imposta la partizione del launcher come bootable
    if (esp_ota_set_boot_partition(launcherPartition) != ESP_OK) {
        //USBSerial.println("Errore: impossibile cambiare partizione di boot");
        return;
    }

    //USBSerial.println("Riavvio per tornare al launcher...");
    delay(500);
    esp_restart();  // Riavvia ESP32, partirà dal launcher
}
