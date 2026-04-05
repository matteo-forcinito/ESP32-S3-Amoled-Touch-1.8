#include "RadioManager.h"
#include "SD_MMC.h"
#include "AudioManager.h"

std::vector<RadioStation> RadioManager::stations;

AudioGeneratorMP3* RadioManager::mp3 = nullptr;
AudioFileSourceICYStream* RadioManager::stream = nullptr;
AudioOutputESP32I2S* RadioManager::output = nullptr;

TaskHandle_t RadioManager::taskHandle = NULL;

int RadioManager::requestedIndex = -1;
int RadioManager::currentIndex = -1;

 std::atomic<bool> RadioManager::requestChange = false;
 std::atomic<bool> RadioManager::requestStop = false;
 std::atomic<bool> RadioManager::requestClose = false;
 unsigned long RadioManager::lastTime = 0;

RadioManager::State RadioManager::state = RadioManager::State::STOPPED;


// ================= INIT =================

bool RadioManager::init() {
    return loadFromSD();
}

void RadioManager::deinit() {
    requestClose = true;
}



RadioManager::State RadioManager::getState() {
    return state;
}


// ================= SD LOAD =================

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
    return !stations.empty();
}


// ================= GETTERS =================

std::vector<RadioStation>& RadioManager::getAll() {
    return stations;
}

bool RadioManager::getById(uint16_t id, RadioStation& out) {
    for (auto& s : stations) {
        if (s.id == id) {
            out = s;
            return true;
        }
    }
    return false;
}


// ================= CONTROL =================

bool RadioManager::play(uint16_t id) {
    if (stations.empty()) return false;

    if(stream) stream->close();

    int index = -1;
    for (int i = 0; i < stations.size(); i++) {
        if (stations[i].id == id) {
            index = i;
            break;
        }
    }

    //if (index < 0) return false;

    requestedIndex = index;
    requestChange = true;   // 🔥 QUESTO MANCAVA
    requestClose = false;

    return true;
}

void RadioManager::stop() {
    requestStop = true;
    if (taskHandle) {
        xTaskNotifyGive(taskHandle);  // sveglia subito
    }
    AudioManager::deinit();
}

bool RadioManager::isPlaying() {
    return state == State::PLAYING;
}

bool RadioManager::start() {
    AudioManager::init();
    if (!taskHandle) {
        xTaskCreatePinnedToCore(
        RadioManager::radioTask,
        "RadioTask",
        8192,      // stack size
        nullptr,
        1,         // priority
        &RadioManager::taskHandle,
        1          // core
        );
    }
    if(taskHandle) {
        return true;
    }
    return false;
}

// ================= TASK =================

void RadioManager::radioTask(void* param) {
    while (!requestClose) {

        // aspetta eventuale notifica (wake-up immediato)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));

        if (requestStop) {
            if (stream) stream->close();
            if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
            if (stream) { stream->close(); delete stream; stream = nullptr; }
            if (output) { delete output; output = nullptr; }

            state = State::STOPPED;
            requestStop = false;
        }

        if (WiFi.status() != WL_CONNECTED) {
            if (mp3) mp3->stop();
            state = State::STOPPED;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // STOP richiesto
        if (requestStop) {
            if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
            if (stream) { stream->close(); delete stream; stream = nullptr; }
            if (output) { delete output; output = nullptr; }

            state = State::STOPPED;
            currentIndex = -1;
            requestStop = false;
        }

        // Cambio stazione o reconnect
        if (requestChange || state != State::PLAYING) {
            requestChange = false;

            if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
            if (stream) { stream->close(); delete stream; stream = nullptr; }
            if (output) { delete output; output = nullptr; }

            currentIndex = requestedIndex;
            if (currentIndex < 0) {
                requestStop = true;
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            state = State::CHANGING;

            stream = new AudioFileSourceICYStream(stations[currentIndex].url.c_str());
            output = new AudioOutputESP32I2S();
            output->SetGain(AudioManager::getVolume() / 100.0f);
            mp3 = new AudioGeneratorMP3();

            if (!mp3->begin(stream, output)) {
                // fallimento → riprova tra 1s
                delete mp3; mp3 = nullptr;
                delete stream; stream = nullptr;
                delete output; output = nullptr;
                state = State::CONNECTING;
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            state = State::PLAYING;
        }

        // Loop audio
        if (state == State::PLAYING && mp3) {
            if (!mp3->loop()) {
                mp3->stop(); delete mp3; mp3 = nullptr;
                stream->close(); delete stream; stream = nullptr;
                delete output; output = nullptr;
                state = State::CONNECTING; // riprova automaticamente
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Cleanup finale
    if (mp3) { mp3->stop(); delete mp3; }
    if (stream) { stream->close(); delete stream; }
    if (output) { delete output; }

    AudioManager::deinit();
    taskHandle = NULL;
    vTaskDelete(NULL);
}

void RadioManager::loop() {
    // limit loop frequency a ~100Hz
    if (millis() - lastTime < 10) return;
    lastTime = millis();

    // 🔴 CHIUSURA STREAM
    if (requestClose) {
        if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
        if (stream) { stream->close(); delete stream; stream = nullptr; }
        if (output) { delete output; output = nullptr; }

        AudioManager::deinit();
        state = State::STOPPED;
        currentIndex = -1;
        return;
    }

    // 🔵 WiFi non connesso: stop e riprova al prossimo loop
    if (WiFi.status() != WL_CONNECTED) {
        if (mp3) mp3->stop();
        state = State::STOPPED;
        return;
    }

    // 🔴 STOP richiesto
    if (requestStop) {
        if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
        if (stream) { stream->close(); delete stream; stream = nullptr; }
        if (output) { delete output; output = nullptr; }

        state = State::STOPPED;
        currentIndex = -1;
        requestStop = false;
        return;
    }

    // 🔵 Cambio stazione richiesto
    if (requestChange) {
        requestChange = false;

        // pulizia risorse precedenti
        if (mp3) { mp3->stop(); delete mp3; mp3 = nullptr; }
        if (stream) { stream->close(); delete stream; stream = nullptr; }
        if (output) { delete output; output = nullptr; }

        currentIndex = requestedIndex;
        if (currentIndex < 0) return;

        state = State::CONNECTING;
        lastTime = 0; // reset timer reconnect
    }

    // 🔹 STATO CONNECTING: tenta di creare lo stream in modo non-bloccante
    if (state == State::CONNECTING) {
        static unsigned long lastConnectAttempt = 0;
        if (millis() - lastConnectAttempt < 200) return; // retry lento 200ms
        lastConnectAttempt = millis();

        if (mp3) return; // già tentato

        stream = new AudioFileSourceICYStream(stations[currentIndex].url.c_str());
        output = new AudioOutputESP32I2S();
        output->SetGain(AudioManager::getVolume() / 100.0f);
        mp3 = new AudioGeneratorMP3();

        if (!mp3->begin(stream, output)) {
            delete mp3; mp3 = nullptr;
            delete stream; stream = nullptr;
            delete output; output = nullptr;
            return; // retry al prossimo loop
        }

        state = State::PLAYING;
    }

    // 🔹 STATO PLAYING: loop breve, non-bloccante
    if (state == State::PLAYING && mp3) {
        bool ok = mp3->loop();
        if (!ok) {
            // se fallisce, pulisci risorse ma non bloccare la UI
            mp3->stop(); delete mp3; mp3 = nullptr;
            stream->close(); delete stream; stream = nullptr;
            delete output; output = nullptr;

            state = State::CONNECTING; // riprova al prossimo loop
        }
    }
}
void RadioManager::stopImmediate() {
    requestStop = true;        // indica al task di fermarsi
    if (taskHandle) {
        // sveglia il task subito invece di aspettare il delay
        xTaskNotifyGive(taskHandle);
    }
}