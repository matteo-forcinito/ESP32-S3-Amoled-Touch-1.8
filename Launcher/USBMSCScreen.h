#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>
#include "SDManager.h"

extern SDManager sdManager;

#define BLOCK_SIZE 512

static USBMSC usb_msc;
static bool sd_mounted_for_msc = false;

static void msc_ensure_mounted() {
    if (!sd_mounted_for_msc) {
        SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
        SD_MMC.begin("/sdcard", true);
        sd_mounted_for_msc = true;
    }
}

static int32_t msc_read(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    msc_ensure_mounted();

    uint8_t* buf = (uint8_t*)buffer;
    uint32_t sector = (lba * BLOCK_SIZE + offset) / BLOCK_SIZE;
    uint32_t sector_offset = (lba * BLOCK_SIZE + offset) % BLOCK_SIZE;
    uint32_t remaining = bufsize;
    uint32_t copied = 0;
    uint8_t temp[BLOCK_SIZE];

    while (remaining > 0) {
        if (!SD_MMC.readRAW(temp, sector)) return -1;
        uint32_t chunk = min(remaining, BLOCK_SIZE - sector_offset);
        memcpy(buf + copied, temp + sector_offset, chunk);
        remaining -= chunk;
        copied += chunk;
        sector++;
        sector_offset = 0;
    }
    return bufsize;
}

static int32_t msc_write(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    msc_ensure_mounted();

    uint32_t sector = (lba * BLOCK_SIZE + offset) / BLOCK_SIZE;
    uint32_t sector_offset = (lba * BLOCK_SIZE + offset) % BLOCK_SIZE;
    uint32_t remaining = bufsize;
    uint32_t written = 0;
    uint8_t temp[BLOCK_SIZE];

    while (remaining > 0) {
        if (!SD_MMC.readRAW(temp, sector)) return -1;
        uint32_t chunk = min(remaining, BLOCK_SIZE - sector_offset);
        memcpy(temp + sector_offset, buffer + written, chunk);
        if (!SD_MMC.writeRAW(temp, sector)) return -1;
        remaining -= chunk;
        written += chunk;
        sector++;
        sector_offset = 0;
    }
    return bufsize;
}

static bool msc_start_stop(uint8_t power_condition, bool start, bool load_eject) {
    if (!start) {
        SD_MMC.end();
        sd_mounted_for_msc = false;
    } else {
        msc_ensure_mounted();
    }
    return true;
}

class USBMSCScreen : public AppScreen {
public:
    void onCreate() override {
        // 1. Smonta SD dall'ESP32 (era in uso dal sistema)
        sdManager.unmount();
        sd_mounted_for_msc = false;

        // 2. Rimonta solo per leggere la dimensione, poi smonta di nuovo
        SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
        SD_MMC.begin("/sdcard", true);
        uint64_t blockCount = SD_MMC.cardSize() / BLOCK_SIZE;
        SD_MMC.end();

        // 3. Registra callback e avvia MSC
        usb_msc.onRead(msc_read);
        usb_msc.onWrite(msc_write);
        usb_msc.onStartStop(msc_start_stop);
        usb_msc.mediaPresent(true);
        usb_msc.begin(blockCount, BLOCK_SIZE);

        // 4. USB.begin() solo se non già avviato
        // Se hai USB CDC attivo nel setup(), commentare questa riga
        USB.begin();
    }

    void onDestroy() override {
        // Smonta lato MSC e riavvia per ripristinare lo stato SD pulito
        usb_msc.mediaPresent(false);
        SD_MMC.end();
        sd_mounted_for_msc = false;
        esp_restart();
    }
};