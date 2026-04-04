#pragma once
#include <Arduino.h>
#include "ESP_I2S.h"
#include "es8311.h"

class AudioManager {
public:
    static void init();
    static void deinit();

    static void setVolume(uint8_t vol);     // 0–100
    static void volumeUp(uint8_t step = 5);
    static void volumeDown(uint8_t step = 5);
    static uint8_t getVolume();

    static void mute();
    static void unmute();

    static void playTap();   // feedback breve
    // nuova versione non bloccante
    static void startAlarm();
    static void stopAlarm();
    static void update(); // da chiamare ogni loop()

    static void ampOn();
    static void ampOff();

    static void alarmTask(void* param);

    static bool isAlarmRunning() { return alarmRunning; }
private:
    static bool initialized;
    static uint8_t volume;
    static bool alarmPlaying;
    static uint16_t alarmDuration; // secondi
    static uint32_t alarmStartMillis;
    static int sampleIndex;
    static const int freq = 1000;
    static const int sampleRate = 16000;
    static const int amplitude = 2000;
    static TaskHandle_t alarmTaskHandle;
    static bool alarmRunning;
};