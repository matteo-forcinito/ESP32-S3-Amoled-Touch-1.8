#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>

#define BLOCK_SIZE 512

USBMSC usb_msc;

// ================= CALLBACK =================

int32_t msc_read(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    uint8_t* buf = (uint8_t*)buffer;
    uint32_t addr = lba * BLOCK_SIZE + offset;

    uint32_t sector = addr / BLOCK_SIZE;
    uint32_t sector_offset = addr % BLOCK_SIZE;
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

int32_t msc_write(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    uint32_t addr = lba * BLOCK_SIZE + offset;
    uint32_t sector = addr / BLOCK_SIZE;
    uint32_t sector_offset = addr % BLOCK_SIZE;

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

bool msc_start_stop(uint8_t power_condition, bool start, bool load_eject) {
    if (!start) SD_MMC.end();     // smonta lato ESP
    else SD_MMC.begin();          // rimonta
    return true;
}

// ================= SCREEN =================

class USBMSCScreen : public AppScreen {
public:
    void onCreate() override {
        //USBSerial.println("[USBMSC] Init...");

        if (!SD_MMC.begin()) {
            //USBSerial.println("[USBMSC] ❌ SD init failed");
            return;
        }

        uint32_t cardSize = SD_MMC.cardSize(); // bytes
        uint32_t blockCount = cardSize / BLOCK_SIZE;

        //USBSerial.printf("[USBMSC] Size: %u blocks\n", blockCount);

        USB.begin();

        usb_msc.onRead(msc_read);
        usb_msc.onWrite(msc_write);
        usb_msc.onStartStop(msc_start_stop);

        usb_msc.begin(blockCount, BLOCK_SIZE);

        //USBSerial.println("[USBMSC] ✅ Ready");
    }

    void onDestroy() override {
        esp_restart();
    }
};