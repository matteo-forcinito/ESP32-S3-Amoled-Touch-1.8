#include "SDManager.h"

bool SDManager::init() {
    return mount();
}

bool SDManager::mount() {
    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_DATA);
    if (!SD_MMC.begin("/sdcard", true)) {
        initialized = false;
        return false;
    }
    if (SD_MMC.cardType() == CARD_NONE) {
        initialized = false;
        return false;
    }
    initialized = true;
    return true;
}

void SDManager::unmount() {
    SD_MMC.end();
    initialized = false;
}

uint64_t SDManager::cardSizeBytes() {
    if (!initialized) return 0;
    return SD_MMC.cardSize();  // già in bytes su arduino-esp32 >= 2.x
}

std::vector<String> SDManager::listFiles(const char *path, uint8_t levels) {
    std::vector<String> files;
    if (!initialized) {
        //USBSerial.println("[SDManager] ⚠️ SD card not initialized");
        return files;
    }

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) {
        //USBSerial.printf("[SDManager] ❌ Cannot open directory: %s\n", path);
        return files;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            //USBSerial.printf("DIR : %s\n", file.name());
            if (levels) {
                // Ricorsione
                auto subFiles = listFiles(file.path(), levels - 1);
                files.insert(files.end(), subFiles.begin(), subFiles.end());
            }
        } else {
            String filePath = String(file.path());
            //USBSerial.printf("FILE: %s (%u bytes)\n", filePath.c_str(), (unsigned int)file.size());
            files.push_back(filePath);
        }
        file = root.openNextFile();
    }

    return files;
}

std::vector<FileEntry> SDManager::listFolder(const char *path) {
    std::vector<FileEntry> list;

    if (!initialized) return list;

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) return list;

    File entry = root.openNextFile();
    while (entry) {
        FileEntry e;

        String fullName = String(entry.name());
        e.name = fullName.substring(fullName.lastIndexOf('/') + 1);
        e.isDirectory = entry.isDirectory();
        e.size = entry.size();

        list.push_back(e);

        entry = root.openNextFile();
    }

    return list;
}

std::vector<String> SDManager::listAppFolders(const char *path) {
    std::vector<String> apps;

    if (!initialized) {
        //USBSerial.println("[SDManager] ⚠️ SD card not initialized");
        return apps;
    }

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) {
        //USBSerial.printf("[SDManager] ❌ Cannot open directory: %s\n", path);
        return apps;
    }

    File folder = root.openNextFile();
    while (folder) {
        if (folder.isDirectory()) {
            String folderName = String(folder.name());
            //USBSerial.printf("[SDManager] 📁 Found folder: %s\n", folderName.c_str());

            // Costruisci il percorso completo del bin
            String expectedBin = String(path) + "/" + folderName + "/" + folderName + ".bin";

            // Verifica se esiste l'eseguibile
            if (SD_MMC.exists(expectedBin.c_str())) {
                apps.push_back(folderName);
                //USBSerial.printf("[SDManager] ✅ App valida trovata: %s\n", folderName.c_str());
            } else {
                //USBSerial.printf("[SDManager] ⚠️ Nessun .bin valido in: %s\n", folderName.c_str());
            }
        }
        folder = root.openNextFile();
    }

    return apps;
}

File SDManager::open(const char* path) {
    if (!isReady()) init();
    return SD_MMC.open(path);
}

bool SDManager::isPathExists(const char *path) {
    return isReady() && SD_MMC.exists(path);
}