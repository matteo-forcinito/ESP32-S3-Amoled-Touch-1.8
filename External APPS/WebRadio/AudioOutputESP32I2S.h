#pragma once
#include "AudioOutput.h"
#include "ESP_I2S.h"

extern I2SClass i2s;

class AudioOutputESP32I2S : public AudioOutput {
public:
    bool begin() override { return true; }
    bool stop() override { return true; }
    void flush() override {}

    bool SetRate(int hz) override {
        sampleRate = hz;
        return true;
    }

    bool SetChannels(int ch) override {
        channels = ch;
        return true;
    }

    bool ConsumeSample(int16_t sample[2]) override {
        // stereo 16bit
        i2s.write((uint8_t*)sample, 4);
        return true;
    }

private:
    int sampleRate = 44100;
    int channels = 2;
};