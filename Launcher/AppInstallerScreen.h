#pragma once
#include "AppScreen.h"
#include <Update.h>
#include <lvgl.h>
#include <SD_MMC.h>

extern SemaphoreHandle_t g_sdMutex;

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

public:
    AppInstallerScreen(const String &name) : appName(name) {
        id = APP_INSTALLER;
    }

    void onCreate() override {
        lv_obj_set_style_bg_color(root, lv_color_black(), 0);
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *title = lv_label_create(root);
        lv_label_set_text_fmt(title, "Installer: %s", appName.c_str());
        lv_obj_set_style_text_color(title, lv_color_white(), 0);

        icon = lv_img_create(root);
        loadAppIcon();

        labelStatus = lv_label_create(root);
        lv_label_set_text(labelStatus, "Verifica SD...");
        lv_obj_set_style_text_color(labelStatus, lv_color_white(), 0);

        progressBar = lv_bar_create(root);
        lv_obj_set_size(progressBar, 180, 12);
        lv_obj_add_flag(progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_range(progressBar, 0, 100);

        xTaskCreatePinnedToCore(checkAppTaskStatic, "CheckApp", 4096, this, 1, NULL, 0);
    }

private:
    // ================= TASK CHECK =================

    static void checkAppTaskStatic(void *param) {
        auto *self = (AppInstallerScreen*)param;
        self->checkAppTask();
        vTaskDelete(NULL);
    }

    void checkAppTask() {
        String folder = "/apps/" + appName;
        String binPath = folder + "/" + appName + ".bin";

        xSemaphoreTake(g_sdMutex, portMAX_DELAY);
        bool folderExists = SD_MMC.exists(folder.c_str());
        bool fileExists = SD_MMC.exists(binPath.c_str());
        xSemaphoreGive(g_sdMutex);

        if (!folderExists) {
            updateStatus("Cartella non trovata");
            return;
        }

        if (!fileExists) {
            updateStatus("File .bin non trovato");
            return;
        }

        appFound = true;
        updateStatus("App trovata. Installazione...");

        lv_async_call([](void* p){
            auto *ctx = (AppInstallerScreen*)p;
            lv_obj_clear_flag(ctx->progressBar, LV_OBJ_FLAG_HIDDEN);
        }, this);

        xTaskCreatePinnedToCore(installAppTaskStatic, "OTAInstall", 16384, this, 1, NULL, 1);
    }

    // ================= ICON =================

    void loadAppIcon() {
        String path = "/apps/" + appName + "/icon.bin";

        xSemaphoreTake(g_sdMutex, portMAX_DELAY);
        bool exists = SD_MMC.exists(path.c_str());
        xSemaphoreGive(g_sdMutex);

        if (!exists) {
            path = "/assets/icons/placeholder.bin";
        }

        String lvPath = "S:" + path;
        lv_img_set_src(icon, lvPath.c_str());
        lv_obj_set_size(icon, 64, 64);
    }

    // ================= TASK INSTALL =================

    static void installAppTaskStatic(void *param) {
        auto *self = (AppInstallerScreen*)param;
        self->installAppTask();
        vTaskDelete(NULL);
    }

    void installAppTask() {
        if (!appFound || isInstalling) return;
        isInstalling = true;

        String binPath = "/apps/" + appName + "/" + appName + ".bin";

        File f;

        xSemaphoreTake(g_sdMutex, portMAX_DELAY);
        f = SD_MMC.open(binPath.c_str());
        xSemaphoreGive(g_sdMutex);

        if (!f) {
            updateStatus("Errore apertura file");
            isInstalling = false;
            return;
        }

        size_t size = f.size();
        if (size < 100000) {
            updateStatus("BIN non valido");
            f.close();
            isInstalling = false;
            return;
        }

        if (!Update.begin(size)) {
            updateStatus("Update.begin fallito");
            f.close();
            isInstalling = false;
            return;
        }

        uint8_t buf[1024];
        size_t written = 0;
        int lastProgress = -1;

        while (true) {
            xSemaphoreTake(g_sdMutex, portMAX_DELAY);
            int available = f.available();
            if (!available) {
                xSemaphoreGive(g_sdMutex);
                break;
            }

            size_t r = f.read(buf, sizeof(buf));
            xSemaphoreGive(g_sdMutex);

            if (r == 0) break;

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
                    auto *ctx = (AppInstallerScreen*)p;
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
        currentStatus = "Installazione completata!";

        lv_async_call([](void* p){
            auto *ctx = (AppInstallerScreen*)p;
            lv_bar_set_value(ctx->progressBar, 100, LV_ANIM_OFF);
            lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
        }, this);

        vTaskDelay(pdMS_TO_TICKS(800));
        esp_restart();
    }

    // ================= UI =================

    void updateStatus(const String &msg) {
        currentStatus = msg;

        lv_async_call([](void* p){
            auto *ctx = (AppInstallerScreen*)p;
            lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
        }, this);
    }
};