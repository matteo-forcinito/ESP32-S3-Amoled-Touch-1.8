#include "RadioManager.h"
#include "SD_MMC.h"

std::vector<RadioStation> RadioManager::stations;
int RadioManager::currentIndex = -1;

AudioGeneratorMP3 RadioManager::mp3;
AudioFileSourceICYStream RadioManager::stream;
AudioOutputESP32I2S RadioManager::output;

bool RadioManager::running = false;

bool RadioManager::init() {
    return loadFromSD();
}

bool RadioManager::loadFromSD() {
    File file = SD_MMC.open("/stations.txt");
    if (!file) return false;

    stations.clear();

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) continue;

        int p1 = line.indexOf('|');
        int p2 = line.indexOf('|', p1 + 1);

        if (p1 < 0 || p2 < 0) continue;

        RadioStation s;
        s.id = line.substring(0, p1).toInt();
        s.name = line.substring(p1 + 1, p2);
        s.url = line.substring(p2 + 1);

        stations.push_back(s);
    }

    file.close();
    return stations.size() > 0;
}

std::vector<RadioStation>& RadioManager::getAll() {
    return stations;
}

RadioStation* RadioManager::getById(uint16_t id) {
    USBSerial.print("getById");
    for (auto& s : stations) {
        if (s.id == id) return &s;
    }
    USBSerial.print("id not found");
    return nullptr;
}

bool RadioManager::play(uint16_t id) {
    RadioStation* s = getById(id);
    if (!s) return false;

    stop();

    USBSerial.print("Opening stream for: "); Serial.println(s->url);
    stream.open(s->url.c_str());
    mp3.begin(&stream, &output);

    for (int i = 0; i < stations.size(); i++) {
        if (stations[i].id == id) {
            currentIndex = i;
            break;
        }
    }

    running = true;
    return true;
}

void RadioManager::stop() {
    if (running) {
        mp3.stop();
        stream.close();
        running = false;
    }
}

void RadioManager::update() {
    if (!running) return;

    if (mp3.isRunning()) {
        if (!mp3.loop()) {
            // restart automatico
            stream.close();
            stream.open(stations[currentIndex].url.c_str());
            mp3.begin(&stream, &output);
        }
    }
}

bool RadioManager::isPlaying() {
    return running;
}

bool RadioManager::loadFromSD2(const char* path) {
    stations.clear();

    File file = sdManager.open(path);
    if (!file) {
        // file non trovato
        return false;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();

        // skip righe vuote o commenti
        if (line.length() == 0 || line.startsWith("#")) continue;

        int p1 = line.indexOf('|');
        int p2 = line.indexOf('|', p1 + 1);

        if (p1 < 0 || p2 < 0) {
            // formato invalido
            continue;
        }

        String idStr   = line.substring(0, p1);
        String nameStr = line.substring(p1 + 1, p2);
        String urlStr  = line.substring(p2 + 1);

        idStr.trim();
        nameStr.trim();
        urlStr.trim();

        // validazione minima
        if (idStr.length() == 0 || nameStr.length() == 0 || urlStr.length() == 0) {
            continue;
        }

        RadioStation s;
        s.id = idStr.toInt();
        s.name = nameStr;
        s.url = urlStr;

        stations.push_back(s);
    }

    file.close();

    return stations.size() > 0;
}