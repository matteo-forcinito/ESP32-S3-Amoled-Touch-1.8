#include "TimeManager.h"

bool TimeManager::loadTime() {
    if (!SD_MMC.begin("/sdcard", true)) {
        //USBSerial.println("[TimeManager] ❌ SD non disponibile");
        return false;
    }

    if (!SD_MMC.exists(TIME_FILE)) {
        //USBSerial.println("[TimeManager] ⚠️ Nessun file tempo trovato, uso fallback");
        setFallbackTime();
        return false;
    }

    File file = SD_MMC.open(TIME_FILE, FILE_READ);
    if (!file) {
        //USBSerial.println("[TimeManager] ❌ Impossibile aprire file tempo");
        setFallbackTime();
        return false;
    }

    time_t savedTime = 0;
    file.read((uint8_t*)&savedTime, sizeof(savedTime));
    file.close();

    struct timeval now = { .tv_sec = savedTime };
    settimeofday(&now, nullptr);

    //USBSerial.printf("[TimeManager] 🕒 Tempo ripristinato da SD: %s", ctime(&savedTime));
    return true;
}

void TimeManager::saveTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        //USBSerial.println("[TimeManager] ⚠️ Impossibile leggere orario attuale, non salvo");
        return;
    }

    time_t now;
    time(&now);

    File file = SD_MMC.open(TIME_FILE, FILE_WRITE);
    if (!file) {
        //USBSerial.println("[TimeManager] ❌ Impossibile aprire file tempo per scrittura");
        return;
    }

    file.write((uint8_t*)&now, sizeof(now));
    file.close();
    //USBSerial.printf("[TimeManager] 💾 Tempo salvato: %s", ctime(&now));
}

void TimeManager::setFallbackTime() {
    struct tm tm;
    tm.tm_year = 2025 - 1900; // anno base
    tm.tm_mon  = 0;           // gennaio
    tm.tm_mday = 1;
    tm.tm_hour = 12;
    tm.tm_min  = 0;
    tm.tm_sec  = 0;
    time_t fallback = mktime(&tm);
    struct timeval now = { .tv_sec = fallback };
    settimeofday(&now, nullptr);

    //USBSerial.printf("[TimeManager] ⏰ Impostato tempo di fallback: %s", ctime(&fallback));
}
