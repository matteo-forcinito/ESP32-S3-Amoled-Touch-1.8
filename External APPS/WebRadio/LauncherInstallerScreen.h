#pragma once
#include "AppScreen.h"

class LauncherInstallerScreen : public AppScreen {
private:
    lv_obj_t *label;

    unsigned long startTime = 0;
public:
    void onCreate() override {
        label = lv_label_create(root);
        lv_label_set_text(label, "Installing...");
        lv_obj_center(label);

        startTime = millis();
    }

    void returnToLauncher() {
        // Ottieni la partizione di boot attiva (launcher)
        const esp_partition_t* launcherPartition = esp_ota_get_next_update_partition(nullptr);

        if (!launcherPartition) {
            Serial.println("Errore: impossibile trovare la partizione del launcher!");
            return;
        }

        // Imposta la partizione del launcher come bootable
        if (esp_ota_set_boot_partition(launcherPartition) != ESP_OK) {
            Serial.println("Errore: impossibile cambiare partizione di boot");
            return;
        }

        Serial.println("Riavvio per tornare al launcher...");
        delay(500);
        esp_restart();  // Riavvia ESP32, partirà dal launcher
    }

    void onLoop() override {
        if (startTime != 0 && millis() - startTime > 500) { 
            startTime = 0;
            returnToLauncher();
        }
    }
};

