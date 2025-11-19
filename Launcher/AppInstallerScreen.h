#pragma once
#include "AppScreen.h"
#include "SDManager.h"
#include <Update.h>

class AppInstallerScreen : public AppScreen {
private:
    const char *appName;
    lv_obj_t *labelStatus;
    lv_obj_t *progressBar;
    lv_obj_t *icon;

    bool appFound = false;
    bool isInstalling = false;

    // Dati condivisi col task
    int currentProgress = 0;
    String currentStatus = "";

public:
    AppInstallerScreen(const char *name) : appName(name) {
        id = APP_INSTALLER;
    }

    void onCreate() override {
        // Layout principale
        lv_obj_set_style_bg_color(root, lv_color_black(), 0);
        lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(root,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);

        // Titolo
        lv_obj_t *title = lv_label_create(root);
        lv_label_set_text_fmt(title, "Installer: %s", appName);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);

        // Icona — prima tenta quella dell’app, poi placeholder
        icon = lv_img_create(root);
        loadAppIcon();

        // Label di stato
        labelStatus = lv_label_create(root);
        lv_label_set_text(labelStatus, "Ricerca app...");
        lv_obj_set_style_text_color(labelStatus, lv_color_white(), 0);

        // Progress bar
        progressBar = lv_bar_create(root);
        lv_obj_set_size(progressBar, 180, 12);
        lv_obj_add_flag(progressBar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_range(progressBar, 0, 100);

        // Avvio ricerca app
        checkAppOnSD();
    }

    void onDestroy() override {
        // Pulizia eventuale (nessun handle da chiudere)
    }

private:
    // --- Verifica se l'app esiste su SD ---
    void checkAppOnSD() {
        String folder = String("/apps/") + appName;
        String binPath = folder + "/" + appName + ".bin";

        if (!SD_MMC.exists(folder.c_str())) {
            updateStatus("Cartella non trovata!");
            appFound = false;
            return;
        }

        if (!SD_MMC.exists(binPath.c_str())) {
            updateStatus("App non trovata!");
            appFound = false;
            return;
        }

        appFound = true;
        updateStatus("App trovata. Avvio installazione...");
        lv_obj_clear_flag(progressBar, LV_OBJ_FLAG_HIDDEN);

        // Avvia task separato per installazione
        xTaskCreatePinnedToCore(
            installAppTaskStatic,
            "InstallerTask",
            8192,
            this,
            1,
            NULL,
            1 // Core 1 (stesso di LVGL)
        );
    }

    // --- Caricamento icona ---
    void loadAppIcon() {
        String iconPath = String("/apps/") + appName + "/icon.bin";

        if (SD_MMC.exists(iconPath.c_str())) {
            lv_img_set_src(icon, iconPath.c_str());
        } else {
            lv_img_set_src(icon, "S:/assets/icons/placeholder.bin");
        }

        lv_obj_set_size(icon, 64, 64);
    }

    // --- Task wrapper ---
    static void installAppTaskStatic(void *param) {
        AppInstallerScreen *self = static_cast<AppInstallerScreen *>(param);
        self->installAppTask();
        vTaskDelete(NULL);
    }

    // --- Funzione OTA vera e propria ---
    void installAppTask() {
        if (!appFound || isInstalling)
            return;

        isInstalling = true;

        String binPath = String("/apps/") + appName + "/" + appName + ".bin";
        File f = SD_MMC.open(binPath.c_str());
        if (!f) {
            updateStatus("Errore apertura file!");
            isInstalling = false;
            return;
        }

        size_t size = f.size();
        if (!Update.begin(size)) {
            updateStatus("Update.begin fallito!");
            f.close();
            isInstalling = false;
            return;
        }

        uint8_t buf[1024];
        size_t written = 0;

        while (f.available()) {
            size_t r = f.read(buf, sizeof(buf));
            if (Update.write(buf, r) != r) {
                updateStatus("Errore scrittura OTA!");
                Update.end();
                f.close();
                isInstalling = false;
                return;
            }

            written += r;
            int progress = (written * 100) / size;
            currentProgress = progress;
            currentStatus = "Installazione " + String(written / 1024) + "KB / " + String(size / 1024) + "KB";

            // Aggiorna la UI in modo thread-safe
            lv_async_call([](void *p) {
                AppInstallerScreen *ctx = (AppInstallerScreen *)p;
                lv_bar_set_value(ctx->progressBar, ctx->currentProgress, LV_ANIM_OFF);
                lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
            }, this);
        }

        if (!Update.end(true)) {
            updateStatus("Errore al termine OTA!");
            f.close();
            isInstalling = false;
            return;
        }

        f.close();
        currentProgress = 100;
        updateStatus("Installazione completata! Riavvio...");

        lv_async_call([](void *p) {
            AppInstallerScreen *ctx = (AppInstallerScreen *)p;
            lv_bar_set_value(ctx->progressBar, 100, LV_ANIM_OFF);
            lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
        }, this);

        delay(700);
        esp_restart();
    }

    // --- Utility per aggiornare testo in sicurezza ---
    void updateStatus(const String &msg) {
        currentStatus = msg;
        lv_async_call([](void *p) {
            AppInstallerScreen *ctx = (AppInstallerScreen *)p;
            lv_label_set_text(ctx->labelStatus, ctx->currentStatus.c_str());
        }, this);
    }
};
