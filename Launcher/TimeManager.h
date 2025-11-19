#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <time.h>

class TimeManager {
public:
    static bool loadTime();
    static void saveTime();
    static void setFallbackTime();

private:
    static constexpr const char* TIME_FILE = "/system/time.dat";
};
