#include "ConfigManager.h"

ConfigManager::ConfigManager(const char* filename, SemaphoreHandle_t sdMutex)
    : _filename(filename), _sdMutex(sdMutex) {}

void ConfigManager::begin() {
    loadAll();
}

// --- get/set uint16 ---
bool ConfigManager::setUInt16(const char* key, uint16_t value) {
    for (size_t i = 0; i < _count; i++) {
        if (_entries[i].key == key) {
            _entries[i].value = value;
            return saveAll();
        }
    }
    if (_count < 20) {
        _entries[_count++] = {String(key), value};
        return saveAll();
    }
    return false;
}

uint16_t ConfigManager::getUInt16(const char* key, uint16_t defaultValue) {
    for (size_t i = 0; i < _count; i++) {
        if (_entries[i].key == key) return _entries[i].value;
    }
    return defaultValue;
}

// --- bool ---
bool ConfigManager::setBool(const char* key, bool value) {
    return setUInt16(key, value ? 1 : 0);
}

bool ConfigManager::getBool(const char* key, bool defaultValue) {
    return getUInt16(key, defaultValue ? 1 : 0) != 0;
}

// --- internals ---
bool ConfigManager::loadAll() {
    if (!_sdMutex) return false;
    if (xSemaphoreTake(_sdMutex, 500 / portTICK_PERIOD_MS)) {
        if (SD_MMC.exists(_filename)) {
            File f = SD_MMC.open(_filename, FILE_READ);
            if (f) {
                _count = 0;
                while (f.available()) {
                    String line = f.readStringUntil('\n');
                    line.trim();
                    if (line.length() == 0) continue;
                    int eq = line.indexOf('=');
                    if (eq > 0 && _count < 20) {
                        String k = line.substring(0, eq);
                        uint32_t v = line.substring(eq + 1).toInt();
                        _entries[_count++] = {k, v};
                    }
                }
                f.close();
            }
        }
        xSemaphoreGive(_sdMutex);
        return true;
    }
    return false;
}

bool ConfigManager::saveAll() {
    if (!_sdMutex) return false;
    if (xSemaphoreTake(_sdMutex, 500 / portTICK_PERIOD_MS)) {
        File f = SD_MMC.open(_filename, FILE_WRITE);
        if (f) {
            for (size_t i = 0; i < _count; i++) {
                f.printf("%s=%u\n", _entries[i].key.c_str(), _entries[i].value);
            }
            f.close();
            xSemaphoreGive(_sdMutex);
            return true;
        }
        xSemaphoreGive(_sdMutex);
    }
    return false;
}