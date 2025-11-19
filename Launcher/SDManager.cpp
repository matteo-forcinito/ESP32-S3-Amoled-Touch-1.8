#include "SDManager.h"

bool SDManager::init() {
    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("[SDManager] ❌ Card Mount Failed");
        initialized = false;
        return false;
    }

    if (SD_MMC.cardType() == CARD_NONE) {
        Serial.println("[SDManager] ❌ No SD card found");
        initialized = false;
        return false;
    }

    Serial.println("[SDManager] ✅ SD card initialized successfully");
    initialized = true;
    return true;
}

std::vector<String> SDManager::listFiles(const char *path, uint8_t levels) {
    std::vector<String> files;
    if (!initialized) {
        Serial.println("[SDManager] ⚠️ SD card not initialized");
        return files;
    }

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) {
        Serial.printf("[SDManager] ❌ Cannot open directory: %s\n", path);
        return files;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("DIR : %s\n", file.name());
            if (levels) {
                // Ricorsione
                auto subFiles = listFiles(file.path(), levels - 1);
                files.insert(files.end(), subFiles.begin(), subFiles.end());
            }
        } else {
            String filePath = String(file.path());
            Serial.printf("FILE: %s (%u bytes)\n", filePath.c_str(), (unsigned int)file.size());
            files.push_back(filePath);
        }
        file = root.openNextFile();
    }

    return files;
}

std::vector<String> SDManager::listAppFolders(const char *path) {
    std::vector<String> apps;

    if (!initialized) {
        Serial.println("[SDManager] ⚠️ SD card not initialized");
        return apps;
    }

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) {
        Serial.printf("[SDManager] ❌ Cannot open directory: %s\n", path);
        return apps;
    }

    File folder = root.openNextFile();
    while (folder) {
        if (folder.isDirectory()) {
            String folderName = String(folder.name());
            Serial.printf("[SDManager] 📁 Found folder: %s\n", folderName.c_str());

            // Costruisci il percorso completo del bin
            String expectedBin = String(path) + "/" + folderName + "/" + folderName + ".bin";

            // Verifica se esiste l'eseguibile
            if (SD_MMC.exists(expectedBin.c_str())) {
                apps.push_back(folderName);
                Serial.printf("[SDManager] ✅ App valida trovata: %s\n", folderName.c_str());
            } else {
                Serial.printf("[SDManager] ⚠️ Nessun .bin valido in: %s\n", folderName.c_str());
            }
        }
        folder = root.openNextFile();
    }

    return apps;
}

bool SDManager::isPathExists(const char *path) {
    return isReady() && SD_MMC.exists(path);
}