#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include "pin_config.h"

#include "BleManager.h"
#include "ModeManager.h"

// display + touch globals (use your hw objects as earlier)
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"

Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// touch
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus;
std::unique_ptr<Arduino_FT3x68> FT3168;

// LVGL draw buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT / 20];

#define LVGL_TICK_PERIOD_MS 2
void lv_tick_task(void *arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); }

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  if (!FT3168) { data->state = LV_INDEV_STATE_REL; return; }
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  if (fingers > 0) {
    int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
    int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
    if (x < 0) x = 0; if (y < 0) y = 0;
    if (x > LCD_WIDTH) x = LCD_WIDTH; if (y > LCD_HEIGHT) y = LCD_HEIGHT;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else data->state = LV_INDEV_STATE_REL;
}

// Global BleManager pointer for screens callbacks
BleManager *gBleManager = nullptr;

BleManager bleManager;
ModeManager *modeManager = nullptr;

// debounce for button
unsigned long lastButton = 0;
const unsigned long debounceMs = 250;

void setup() {
  Serial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  Serial.println("Starting...");

  // I2C
  Wire.begin(IIC_SDA, IIC_SCL);
  IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
  FT3168.reset(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS));

  // gfx
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(120);

  // LVGL
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, LCD_WIDTH * LCD_HEIGHT / 20);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // input device
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // lv tick timer
  const esp_timer_create_args_t periodic_timer_args = { .callback = &lv_tick_task, .name = "lv_tick" };
  esp_timer_handle_t periodic_timer;
  esp_timer_create(&periodic_timer_args, &periodic_timer);
  esp_timer_start_periodic(periodic_timer, LVGL_TICK_PERIOD_MS * 1000);

  // BLE manager + modes
  gBleManager = &bleManager;
  bleManager.begin();

  modeManager = new ModeManager(&bleManager);
  modeManager->begin();
}

void loop() {
  lv_timer_handler();

  // handle button toggle (PIN 0). Only toggle when pressed and BLE connected
  if (digitalRead(0) == LOW) {
    unsigned long now = millis();
    if (now - lastButton > debounceMs) {
      lastButton = now;
      // toggle mode only if BLE connected
      if (bleManager.isConnected()) modeManager->toggleMode();
      else Serial.println("BLE not connected: toggle ignored");
    }
  }

  // call mode manager loop
  if (modeManager) modeManager->loop();

  delay(5);
}
