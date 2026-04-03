#pragma once

#include <vector>
#include "SDManager.h"
#include "RadioStation.h"
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceICYStream.h"
#include "AudioOutputESP32I2S.h"
#include "HWCDC.h"

extern HWCDC USBSerial;
extern SDManager sdManager;

class RadioManager {
public:
    static bool init();

    static std::vector<RadioStation>& getAll();
    static RadioStation* getById(uint16_t id);

    static bool play(uint16_t id);
    static void stop();
    static void update();

    static bool isPlaying();

private:
    static bool loadFromSD();
    static bool loadFromSD2(const char* path = "/radio/stations.txt");

    static std::vector<RadioStation> stations;
    static int currentIndex;

    static AudioGeneratorMP3 mp3;
    static AudioFileSourceICYStream stream;
    static AudioOutputESP32I2S output;

    static bool running;
};