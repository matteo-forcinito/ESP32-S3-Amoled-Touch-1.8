#pragma once
#include <Arduino.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD_MMC.h>
#include "AppScreenLayout.h"
#include "SDManager.h"

extern SDManager sdManager;

#define BLOCK_SIZE 512

static USBMSC usb_msc;
static bool sd_mounted_for_msc = false;
static bool usb_msc_connected = false;

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
        usb_msc_connected = false;
        SD_MMC.end();
        sd_mounted_for_msc = false;
    } else {
        usb_msc_connected = true;
        msc_ensure_mounted();
    }
    return true;
}

class USBMSCScreen : public AppScreenLayout {
private:
    lv_obj_t *status = nullptr;
    lv_obj_t *imgConn = nullptr;
    lv_obj_t *imgDisconn = nullptr;
    lv_obj_t *content = nullptr;
    bool lastState = false;
    bool mscReady = false;  // ← unico flag necessario

public:
    USBMSCScreen() : AppScreenLayout("USB MSC") {}

    void onCreate() override {
        content = lv_obj_create(container);
        lv_obj_remove_style_all(content);
        lv_obj_set_size(content, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        imgConn = lv_img_create(content);
        lv_obj_set_size(imgConn, 64, 64);
        lv_obj_add_flag(imgConn, LV_OBJ_FLAG_HIDDEN);

        imgDisconn = lv_img_create(content);
        lv_obj_set_size(imgDisconn, 64, 64);
        lv_obj_add_flag(imgDisconn, LV_OBJ_FLAG_HIDDEN); // ← nascosta finché MSC non è pronto

        status = lv_label_create(content);
        lv_label_set_text(status, "In attesa di connessione..");

        // Avvia MSC (SD ancora montata da sdManager qui)
        sdManager.unmount();
        sd_mounted_for_msc = false;

        SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
        SD_MMC.begin("/sdcard", true);
        uint64_t blockCount = SD_MMC.cardSize() / BLOCK_SIZE;
        SD_MMC.end();

        usb_msc.onRead(msc_read);
        usb_msc.onWrite(msc_write);
        usb_msc.onStartStop(msc_start_stop);
        usb_msc.mediaPresent(true);
        usb_msc.begin(blockCount, BLOCK_SIZE);

        USB.onEvent([](void* arg, esp_event_base_t base, int32_t id, void* data) {
            if (id == ARDUINO_USB_STARTED_EVENT) usb_msc_connected = true;
            else if (id == ARDUINO_USB_STOPPED_EVENT) usb_msc_connected = false;
        });

        USB.begin();
        mscReady = true; // ← segnala che al prossimo loop possiamo caricare le immagini
    }

    void onLoop() override {
        // Primo frame dopo MSC pronto: carica le immagini ora che S: è disponibile
        if (mscReady) {
            mscReady = false;
            lv_img_set_src(imgDisconn, "S:/assets/icons/usb-disconnected.bin");
            lv_img_set_src(imgConn, "S:/assets/icons/usb-connected.bin");
            lv_obj_clear_flag(imgDisconn, LV_OBJ_FLAG_HIDDEN); // ← mostra disconnected
            lastState = false;
            return;
        }

        if (lastState == usb_msc_connected) return;

        if (usb_msc_connected) {
            lv_obj_clear_flag(imgConn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(imgDisconn, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(status, "Connesso");
        } else {
            lv_obj_add_flag(imgConn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(imgDisconn, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(status, "In attesa di connessione..");
        }

        lastState = usb_msc_connected;
    }

    void onDestroy() override {
        usb_msc.mediaPresent(false);
        SD_MMC.end();
        sd_mounted_for_msc = false;
        esp_restart();
    }
};