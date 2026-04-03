#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include <FS.h>
#include "pin_config.h"
#include "Utils.h"

class SDManager {
private:
    bool initialized = false;
public:
    bool init();
    std::vector<String> listFiles(const char *path, uint8_t levels = 1);
    std::vector<FileEntry> listFolder(const char *path);
    std::vector<String> listAppFolders(const char *path);
    bool isReady() const { return initialized; }
    bool isPathExists(const char *path);
    File open(const char* path);
};
