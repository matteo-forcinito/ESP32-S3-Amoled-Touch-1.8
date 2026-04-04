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
#include "AppState.h"
#include "lv_fs_sd.h"

#include "AudioManager.h"
extern "C" {
  #include "driver/i2s_std.h"
}
#include "ESP_I2S.h"
I2SClass i2s;

#include "esp_check.h"
#include "es8311.h"
#include "canon.h"

#include "XPowersLib.h"
#include "SensorPCF85063.hpp"

#include "AppState.h"
#include "AudioManager.h"
#include "ScreenManager.h"
#include "RadioManager.h"
#include "HomeNavigation.h"
#include "WebRadioScreen.h"

#define EXAMPLE_SAMPLE_RATE 44100
#define EXAMPLE_VOICE_VOLUME 80                  // 0 - 100
#define EXAMPLE_MIC_GAIN (es8311_mic_gain_t)(3)  // 0 - 7
#define EXAMPLE_RECV_BUF_SIZE (10000)

// Buffer di disegno LVGLgvbff 
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 10];
// LVGL
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1,
  LCD_SDIO2, LCD_SDIO3);

Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// Inizializzazione touch
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
std::unique_ptr<Arduino_FT3x68> FT3168(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS));

#define LVGL_TICK_PERIOD_MS 2


// --- LVGL tick ---
static void lv_tick_task(void *arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); }

// --- LVGL display flush ---
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}

// --- Touch read for FT3168 (returns coordinates in LVGL space) ---
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (!FT3168) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
    int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
    // clamp and map if necessary
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > LCD_WIDTH) x = LCD_WIDTH;
    if (y > LCD_HEIGHT) y = LCD_HEIGHT;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

HWCDC USBSerial;
XPowersPMU power;
SensorPCF85063 rtc;
SemaphoreHandle_t g_sdMutex;

SDManager sdManager;
HomeNavigation home;

uint32_t lastTriggeredId = 0;
unsigned long bootPressedTime = 0;
int brightness = 120;
bool backToLauncher = false;
bool goingBack = false;

void setup() {
  USBSerial.begin(115200);
  pinMode(0, INPUT_PULLUP);

  //USBSerial.println("Startup!");

  if (!i2s.begin(I2S_MODE_STD, EXAMPLE_SAMPLE_RATE,
               I2S_DATA_BIT_WIDTH_16BIT,
               I2S_SLOT_MODE_STEREO,
               I2S_STD_SLOT_BOTH)) {
    USBSerial.println("Failed to initialize I2S bus!");
    return;
  }

  Wire.begin(IIC_SDA, IIC_SCL);

  // display init
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(120);
  // Inizializza PMU
  if (!power.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    //USB////USBSerial.println("Power management init failed!");
  } else {
    //USB////USBSerial.println("Power management ready");
    //isCharging = power.isCharging();
  }
  
  if (!rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    //USB//USBSerial.println("Failed to find PCF8563 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  AudioManager::init();

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  // LVGL init
  lv_init();
  lv_fs_sd_init();

  g_sdMutex = xSemaphoreCreateMutex();
  if(!sdManager.init()) {
    //USBSerial.println("Cannot Mound Card");
  } else {
    //USBSerial.println("Card Mounted");
  }

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
  // 🔊 init audio (USA IL TUO STACK)
  //AudioManager::init();
  RadioManager::init();

  home.open();
}

void loop() {
  lv_timer_handler();
  if(backToLauncher) return;

  ScreenManager screenManager = ScreenManager::get();
  screenManager.loop();

  
  if(digitalRead(0) == LOW && bootPressedTime == 0) {
    bootPressedTime = millis();
  }
  if(digitalRead(0) == HIGH && bootPressedTime != 0) {
    if(millis() - bootPressedTime < 1000) {
      handleAppClose();
    } else {
      backToLauncher = true;
      returnToLauncher();
    }
    bootPressedTime = 0;

    return;
  }
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
        Serial.println("Errore: impossibile trovare la partizione del launcher!");
        return;
    }

    // Imposta la partizione del launcher come bootable
    if (esp_ota_set_boot_partition(launcherPartition) != ESP_OK) {
        Serial.println("Errore: impossibile cambiare partizione di boot");
        return;
    }

    Serial.println("Riavvio per tornare al launcher...");
    delay(500);
    esp_restart();  // Riavvia ESP32, partirà dal launcher
}