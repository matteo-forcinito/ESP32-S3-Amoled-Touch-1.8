#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include <FS.h>
#include "pin_config.h"

class SDManager {
private:
    bool initialized = false;

public:
    bool init();
    std::vector<String> listFiles(const char *path, uint8_t levels = 1);
    std::vector<String> listAppFolders(const char *path);
    bool isReady() const { return initialized; }
    bool isPathExists(const char *path);
};
