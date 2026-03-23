#pragma once
#include <Arduino.h>
#include "ESP_I2S.h"

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

    static void ampOn();
    static void ampOff();

private:
    static bool initialized;
    static uint8_t volume;
};