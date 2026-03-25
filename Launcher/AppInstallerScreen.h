#pragma once
#include "AppScreen.h"
#include "SDManager.h"
#include <Update.h>
#include <lvgl.h>
#include <SD_MMC.h>

class AppInstallerScreen : public AppScreen {
private:
    String appName;
    lv_obj_t *labelStatus;
    lv_obj_t *progressBar;
    lv_obj_t *icon;

    bool appFound = false;
    bool isInstalling = false;

    int currentProgress = 0;
    String currentStatus = "";

    SemaphoreHandle_t sdMutex; // mutex per SD

public:
    AppInstallerScreen(const String &name) : appName(name) {
        id = APP_INSTALLER;
        sdMutex = xSemaphoreCreateMutex();
    }

    void onCreate() override {
        lv_obj_set_style_bg_color(root, lv_color_black(), 0);
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Titolo
        lv_obj_t *title = lv_label_create(root);
        lv_label_set_text_fmt(title, "Installer: %s", appName);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);

        // Icona
        icon = lv_img_create(root);
        loadAppIcon();

        // Label di stato
        labelStatus = lv_label_create(root);
        lv_label_set_text(labelStatus, "Verifica SD...");
        lv_obj_set_style_text_color(labelStatus, lv_color_white(), 0);

        // Progress bar
        progressBar = lv_bar_create(root);
        lv_obj_set_size(progressBar, 180, 12);
        lv_obj_add_flag(progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_range(progressBar, 0, 100);

        // Task di verifica app
        xTaskCreatePinnedToCore(checkAppTaskStatic, "CheckApp", 4096, this, 1, NULL, 0);
    }

    void onDestroy() override {
        if (sdMutex) {
            vSemaphoreDelete(sdMutex);
            sdMutex = nullptr;
        }
    }

private:
    // --- Task statico ---
    static void checkAppTaskStatic(void *param) {
        AppInstallerScreen *self = (AppInstallerScreen*)param;
        self->checkAppTask();
        vTaskDelete(NULL);
    }

    void checkAppTask() {
        xSemaphoreTake(sdMutex, portMAX_DELAY);
        if (!SD_MMC.begin()) {
            updateStatus("Errore inizializzazione SD");
            xSemaphoreGive(sdMutex);
            return;
        }
        xSemaphoreGive(sdMutex);

        String folder = "/apps/" + String(appName);
        String binPath = folder + "/" + String(appName) + ".bin";

        xSemaphoreTake(sdMutex, portMAX_DELAY);
        bool folderExists = SD_MMC.exists(folder.c_str());
        bool fileExists = SD_MMC.exists(binPath.c_str());
        xSemaphoreGive(sdMutex);

        if (!folderExists) {
            updateStatus("Cartella non trovata");
            return;
        }

        if (!fileExists) {
            updateStatus("File .bin non trovato");
            return;
        }

        appFound = true;
        updateStatus("App trovata. Avvio installazione...");

        lv_async_call([](void* p){
            AppInstallerScreen *ctx = (AppInstallerScreen*)p;
            lv_obj_clear_flag(ctx->progressBar, LV_OBJ_FLAG_HIDDEN);
        }, this);

        xTaskCreatePinnedToCore(installAppTaskStatic, "OTAInstall", 16384, this, 1, NULL, 0);
    }

    void loadAppIcon() {
        String iconPath = "S:/apps/" + String(appName) + "/icon.bin";
        xSemaphoreTake(sdMutex, portMAX_DELAY);
        if (!SD_MMC.exists(iconPath.substring(2).c_str())) {
            iconPath = "S:/assets/icons/placeholder.bin"; // fallback
        }
        xSemaphoreGive(sdMutex);

        lv_img_set_src(icon, iconPath.c_str());
        lv_obj_set_size(icon, 64, 64);
    }

    static void installAppTaskStatic(void *param) {
        AppInstallerScreen *self = (AppInstallerScreen*)param;
        self->installAppTask();
        vTaskDelete(NULL);
    }

    void installAppTask() {
        if (!appFound || isInstalling) return;
        isInstalling = true;

        String binPath = "/apps/" + String(appName) + "/" + String(appName) + ".bin";
        File f;

        xSemaphoreTake(sdMutex, portMAX_DELAY);
        f = SD_MMC.open(binPath.c_str());
        xSemaphoreGive(sdMutex);

        if (!f) {
            updateStatus("Errore apertura file");
            isInstalling = false;
            return;
        }

        size_t size = f.size();
        if (!Update.begin(size)) {
            updateStatus("Update.begin fallito");
            f.close();
            isInstalling = false;
            return;
        }

        uint8_t buf[1024];
        size_t written = 0;
        int lastProgress = -1;

        while (f.available()) {
            size_t r = f.read(buf, sizeof(buf));
            if (Update.write(buf, r) != r) {
                updateStatus("Errore scrittura OTA");
                Update.end();
                f.close();
                isInstalling = false;
                return;
            }

            written += r;
            int progress = (written * 100) / size;
            if (progress != lastProgress && progress % 5 == 0) {
                lastProgress = progress;
                currentProgress = progress;
                currentStatus = "Installazione " + String(written/1024) + "KB / " + String(size/1024) + "KB";
                lv_async_call([](void* p){
                    AppInstallerScreen *ctx = (AppInstallerScreen*)p;
                    lv_bar_set_value(ctx->progressBar, ctx->currentProgress, LV_ANIM_OFF);
                    lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
                }, this);
            }

            vTaskDelay(1);
        }

        if (!Update.end(true)) {
            updateStatus("Errore completamento OTA");
            f.close();
            isInstalling = false;
            return;
        }

        f.close();
        currentProgress = 100;
        currentStatus = "Installazione completata! Riavvio...";
        lv_async_call([](void* p){
            AppInstallerScreen *ctx = (AppInstallerScreen*)p;
            lv_bar_set_value(ctx->progressBar, 100, LV_ANIM_OFF);
            lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
        }, this);

        vTaskDelay(pdMS_TO_TICKS(700));
        esp_restart();
    }

    void updateStatus(const String &msg) {
        currentStatus = msg;
        lv_async_call([](void* p){
            AppInstallerScreen *ctx = (AppInstallerScreen*)p;
            lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
        }, this);
    }
};