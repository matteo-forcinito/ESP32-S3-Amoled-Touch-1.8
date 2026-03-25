#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "pin_config.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "SensorPCF85063.hpp"
#include "XPowersLib.h"
#include <esp_sleep.h>

// --- Display (SH8601 QSPI) ---
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);

// --- Touch FT3168 (I2C) ---
// Uses Arduino_FT3x68 class from Arduino_DriveBus_Library
std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus;
std::unique_ptr<Arduino_FT3x68> FT3168;
SensorPCF85063 rtc;
XPowersPMU power;
bool isAlwaysOn = false;
unsigned long bootTime = 0;

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

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  launcher();

}

// --- LOOP ---
void loop() {
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
      //Wire.begin(IIC_SDA, IIC_SCL); // riattiva touch
      setCpuFrequencyMhz(240);
      isAlwaysOn = false;
      gfx->Display_Brightness(140);
      delay(1000);
      launcher();
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
    bootTime = 0;
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

void launcher() {
  gfx->setCursor(50, 50);
  gfx->setTextSize(2);
  gfx->setTextColor(0x7BEF);
  gfx->print("Click boot to enter AlwayOn");

  alwaysOn();
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