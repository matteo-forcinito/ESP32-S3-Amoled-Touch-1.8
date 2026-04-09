#pragma once
#include <Arduino.h>
#include "SDManager.h"
#include "freertos/semphr.h"

class ConfigManager {
public:
    ConfigManager(const char* filename, SemaphoreHandle_t sdMutex);

    void begin();

    // Lettura/scrittura config generica
    bool setUInt16(const char* key, uint16_t value);
    uint16_t getUInt16(const char* key, uint16_t defaultValue = 0);

    bool setBool(const char* key, bool value);
    bool getBool(const char* key, bool defaultValue = false);

private:
    const char* _filename;
    SemaphoreHandle_t _sdMutex;

    struct Entry {
        String key;
        uint32_t value;
    };

    Entry _entries[20]; // max 20 voci
    size_t _count = 0;

    bool loadAll();
    bool saveAll();
};