#define ESP_I2S_h
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
#include <esp_sleep.h>


#include "ScreenManager.h"
#include "BTRemoteApp.h"

#define EXAMPLE_SAMPLE_RATE 16000
#define EXAMPLE_VOICE_VOLUME 80                  // 0 - 100
#define EXAMPLE_MIC_GAIN (es8311_mic_gain_t)(3)  // 0 - 7
#define EXAMPLE_RECV_BUF_SIZE (10000)

const char *TAG = "esp32p4_i2s_es8311";


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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);

  Wire.begin(IIC_SDA, IIC_SCL);
  pinMode(0, INPUT_PULLUP);

  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(120);


  lv_init();

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
  
  ScreenManager::get().changeScreen(new BTRemoteApp());
}

void loop() {
  lv_timer_handler();
  if(digitalRead(0) == LOW) {
    returnToLauncher();
  }

  ScreenManager sm = ScreenManager::get();
  sm.loop();

  int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
  int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  sm.touch(x, y, fingers);
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
