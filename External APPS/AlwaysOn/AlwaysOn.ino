#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
extern "C" {
  #include "driver/i2s_std.h"
}
#include "ESP_I2S.h"
#include "SensorPCF85063.hpp"
#include "XPowersLib.h"
#include <esp_sleep.h>
#include "SDManager.h"
#include "AlwaysOnScreen.h"
#include "LauncherScreen.h"
#include "SimpleScreen.h"
#include "AlarmManager.h"
#include "AlarmTriggerScreen.h"
#include "ConfigManager.h"

#include "HWCDC.h"
HWCDC USBSerial;
// --- Display (SH8601 QSPI) ---
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// --- Touch FT3168 (I2C) ---
// Uses Arduino_FT3x68 class from Arduino_DriveBus_Library
I2SClass i2s;
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus;
std::unique_ptr<Arduino_FT3x68> FT3168;
SensorPCF85063 rtc;
XPowersPMU power;
SemaphoreHandle_t g_sdMutex;
SDManager sdManager;
bool isAlwaysOn = false;
unsigned long bootTime = 0;
SimpleScreen* openApp = nullptr;
AlwaysOnScreen alwaysOnScreen;
LauncherScreen *launcher = new LauncherScreen();
bool enableAlwaysOn = false;
bool backToLaucher = false;
uint32_t lastTriggeredId = 0;
ConfigManager* config = nullptr;

// --- SETUP ---
void setup() {
  USBSerial.begin(115200);
  pinMode(0, INPUT_PULLUP);
  USBSerial.println("Starting...");

  // I2C for touch
  Wire.begin(IIC_SDA, IIC_SCL);
  IIC_Bus = std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);
  FT3168.reset(new Arduino_FT3x68(IIC_Bus, FT3168_DEVICE_ADDRESS));

  // display init
  gfx->begin();
  gfx->fillScreen(BLACK);
  gfx->Display_Brightness(120);

  // Inizializza PMU
  if (!power.begin(Wire, AXP2101_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    //USB//Serial.println("Power management init failed!");
  } else {
    //USB//Serial.println("Power management ready");
  }
  
  if (!rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    //USBSerial.println("Failed to find PCF8563 - check your wiring!");
    while (1) {
      delay(1000);
    }
  }

  g_sdMutex = xSemaphoreCreateMutex();
  if(!sdManager.init()) {
    //USBSerial.println("Cannot Mound Card");
  } else {
    //USBSerial.println("Card Mounted");
  }

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  
  config = new ConfigManager("/config.txt", g_sdMutex);
  config->begin();
  
  AlarmManager::load();
  AlarmManager::initPreferences();

  uint16_t alwaysOnColor = config->getUInt16("alwaysOnColor", 0xF800);
  gfx->setTextColor(alwaysOnColor); // 0x0320  0x03E0  0x0200
  launcher->create();

}
// --- LOOP ---
void loop() {
  checkAlarms();
  if(backToLaucher) {
    backToLaucher = false;
    gfx->fillScreen(BLACK);
    gfx->setCursor(50, 180);
    gfx->setTextSize(3);
    gfx->print("Restarting..");
    delay(500);
    returnToLauncher();

    return;
  }
  RTC_DateTime datetime = rtc.getDateTime();
  if(isAlwaysOn) {
    alwaysOnScreen.alwaysOn();
    
    int seconds = datetime.second;
    int sleepTime = 60 - seconds;
    esp_sleep_enable_timer_wakeup(sleepTime * 1000000ULL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)0, 0);
    esp_light_sleep_start();
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    

    if(wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
      //Wire.begin(IIC_SDA, IIC_SCL); // riattiva touch
      exitAlwaysOn();
      //AudioManager::ampOn();

      return;
    } 
    return;
  }
  if(digitalRead(0) == LOW) {
    if(bootTime == 0) {
      bootTime = millis();
    } else if(millis() - bootTime > 3000) {
      returnToLauncher();
    }
  }
  if(digitalRead(0) == HIGH && bootTime != 0) {
    if(openApp != nullptr) {
      openApp->onDestroy();
      delete openApp;
      openApp = nullptr;
    }
    delete launcher;
    launcher = new LauncherScreen();
    launcher->create();
    bootTime = 0;
  }
  if(enableAlwaysOn) {
    enableAlwaysOn = false;
    isAlwaysOn = true;
    gfx->fillScreen(BLACK);
    gfx->Display_Brightness(20);
    alwaysOnScreen.alwaysOn();
    setCpuFrequencyMhz(80);
  }
  
  int32_t x = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_X);
  int32_t y = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::TOUCH_COORDINATE_Y);
  int32_t fingers = FT3168->IIC_Read_Device_Value(FT3168->Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
  //alwaysOnScreen.touch(x, y, fingers);
  if(openApp != nullptr) {
    openApp->loop();
    openApp->touch(x, y, fingers);
  } else {
    launcher->touch(x, y, fingers);
  }
}


void checkAlarms() {
  time_t now = time(nullptr);

  uint32_t idx = AlarmManager::checkAlarms(now);
  USBSerial.print("idx = ");
  USBSerial.println(String(idx));
  if (idx != 0) {
      const Alarm* a = AlarmManager::getById(idx);
      if(a) {
        USBSerial.printf("id = %s", String(a->id));
        USBSerial.printf("idx = %s", String(idx));
      }
      if (a && a->id != lastTriggeredId) {
          if(isAlwaysOn) exitAlwaysOn();

          lastTriggeredId = a->id;
          if (openApp) {
            openApp->onDestroy();
            delete openApp;
            openApp = nullptr;
          }

          openApp = new AlarmTriggerScreen(a->id);
          openApp->create();
      }
  }
}

void exitAlwaysOn() {
  setCpuFrequencyMhz(240);
  isAlwaysOn = false;
  gfx->Display_Brightness(140);
  delay(1000);
  alwaysOnScreen.create();
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