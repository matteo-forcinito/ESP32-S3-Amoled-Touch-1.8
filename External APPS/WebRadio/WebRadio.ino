/* BTKeyboard - fixed layout + touch + safe callbacks
   ESP32-S3 Waveshare 1.8" (368x448)
*/

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>

#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
//#include "AudioManager.h"
#include "ScreenManager.h"
#include "WebRadioScreen.h"

// --- Display (SH8601 QSPI) ---
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// --- Touch FT3168 (I2C) ---
// Uses Arduino_FT3x68 class from Arduino_DriveBus_Library
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus;
std::unique_ptr<Arduino_FT3x68> FT3168;

// small LVGL draw buffer to save RAM
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 20];

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

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  Serial.println("Starting...");

  // I2C for touch
  Wire.begin(IIC_SDA, IIC_SCL);
  IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
  FT3168.reset(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS));

  // display init
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(120);

  // LVGL init
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_WIDTH * LCD_HEIGHT / 20);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // input device (touch)
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // lv_tick timer
  const esp_timer_create_args_t periodic_timer_args = { .callback = &lv_tick_task, .name = "lv_tick" };
  esp_timer_handle_t periodic_timer;
  esp_timer_create(&periodic_timer_args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, LVGL_TICK_PERIOD_MS * 1000);

  // initial status label
  lv_obj_t* label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello, World!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  //AudioManager::init();

  ScreenManager::get().changeScreen(new WebRadioScreen());
}

// --- LOOP ---
void loop() {
  lv_timer_handler();
  if(digitalRead(0) == LOW) {
    returnToLauncher();
  }

  ScreenManager screenManager = ScreenManager::get();
  screenManager->loop();
  
  int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
  int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if(fingers > 0) {
    // actually disabled due to a bad feedback sound
    // AudioManager::playTap();
  }

  screenManager.touch(x, y, fingers);
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
