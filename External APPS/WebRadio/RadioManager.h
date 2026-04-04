#pragma once

#include <vector>
#include <Arduino.h>
#include <WiFi.h>
#include "HWCDC.h"
#include "RadioStation.h"
#include <atomic>

#include "AudioFileSourceICYStream.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputESP32I2S.h"

extern HWCDC USBSerial;

class RadioManager {
public:
    static bool init();
    static void deinit();
    static bool start();

    static bool play(uint16_t id);
    static void stop();

    static bool isPlaying();

    static std::vector<RadioStation>& getAll();
    static bool getById(uint16_t id, RadioStation& out);

    static void stopImmediate();

    static void loop();

    enum class State {
        STOPPED,
        CONNECTING,
        PLAYING,
        CHANGING,
        INIT
    };

    static State getState();

private:
    static void radioTask(void* param);
    static bool loadFromSD();

    // data
    static std::vector<RadioStation> stations;

    // audio
    static AudioGeneratorMP3* mp3;
    static AudioFileSourceICYStream* stream;
    static AudioOutputESP32I2S* output;

    // task
    static TaskHandle_t taskHandle;

    // state
    static int requestedIndex;
    static int currentIndex;

    static std::atomic<bool> requestStop;
    static std::atomic<bool> requestChange;
    static std::atomic<bool> requestClose;
    static unsigned long lastTime;

    static State state;
};