#include "AudioManager.h"
#include "pin_config.h"
#include "FS.h"
#include "SD_MMC.h"

extern I2SClass i2s;
static es8311_handle_t es_handle = NULL;

bool AudioManager::initialized = false;
uint8_t AudioManager::volume = 80;
bool AudioManager::alarmPlaying = false;
uint16_t AudioManager::alarmDuration = 0;
uint32_t AudioManager::alarmStartMillis = 0;
int AudioManager::sampleIndex = 0;
TaskHandle_t AudioManager::alarmTaskHandle = NULL;
bool AudioManager::alarmRunning = false;


void AudioManager::alarmTask(void* param) {
    const int bufferSize = 512;
    uint8_t buffer[bufferSize];

    // --- Parametri fallback (tone sintetico) ---
    const int sampleRate = 16000;
    float phase = 0;
    float freq = 800.0;

    while (alarmRunning) {

        File file = SD_MMC.open("/alarm.wav");

        if (file) {
            // --- WAV playback ---
            file.seek(44); // skip header

            while (alarmRunning && file.available()) {
                int len = file.read(buffer, bufferSize);
                i2s.write(buffer, len);
            }

            file.close();

        } else {
            // --- FALLBACK: tono sintetico ---
            int16_t synthBuffer[256];

            for (int i = 0; i < 256; i++) {
                float sample = sin(phase);

                // piccolo sweep per renderlo meno "piatto"
                freq += 0.3;
                if (freq > 1400) freq = 800;

                float phaseIncrement = 2 * PI * freq / sampleRate;

                synthBuffer[i] = (int16_t)(sample * 3000);

                phase += phaseIncrement;
                if (phase > 2 * PI) phase -= 2 * PI;
            }

            i2s.write((uint8_t*)synthBuffer, sizeof(synthBuffer));

            // evita saturazione CPU
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // pulizia finale (evita click)
    //i2s.zeroDMA();

    alarmRunning = false;
    vTaskDelete(NULL);
}

void AudioManager::startAlarm() {
    if (alarmRunning) return;

    if (!initialized) init();

    alarmRunning = true;

    xTaskCreatePinnedToCore(
        alarmTask,
        "AlarmTask",
        4096,
        NULL,
        1,
        &alarmTaskHandle,
        1
    );
}

void AudioManager::stopAlarm() {
    if (!alarmRunning) return;

    alarmRunning = false;

    // aspetta che il task si chiuda
    if (alarmTaskHandle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(50));
        alarmTaskHandle = NULL;
    }
}

void AudioManager::update() {
    if (!alarmPlaying) return;

    // calcoliamo il tempo trascorso
    uint32_t elapsed = (millis() - alarmStartMillis) / 1000;
    if (elapsed >= alarmDuration) {
        alarmPlaying = false;
        return;
    }

    const int samplesPerCycle = sampleRate / freq;
    int16_t buffer[64]; // piccolo buffer per non bloccare

    // generiamo pochi campioni per ogni chiamata
    for (int i = 0; i < 64; i++) {
        buffer[i] = (sampleIndex < samplesPerCycle / 2) ? amplitude : -amplitude;
        sampleIndex++;
        if (sampleIndex >= samplesPerCycle) sampleIndex = 0;
    }

    i2s.write((uint8_t*)buffer, sizeof(buffer));
}

void AudioManager::init() {
    if (initialized) return;

    pinMode(PA, OUTPUT);
    digitalWrite(PA, HIGH); // amp ON

    i2s.setPins(BCLKPIN, WSPIN, DIPIN, DOPIN, MCLKPIN);

    if (!i2s.begin(I2S_MODE_STD, 16000,
                   I2S_DATA_BIT_WIDTH_16BIT,
                   I2S_SLOT_MODE_STEREO,
                   I2S_STD_SLOT_BOTH)) {
        //USBSerial.println("I2S init failed");
        return;
    }

    es_handle = es8311_create(0, ES8311_ADDRRES_0);

    es8311_clock_config_t clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = 16000 * 256,
        .sample_frequency = 16000
    };

    es8311_init(es_handle, &clk,
                ES8311_RESOLUTION_16,
                ES8311_RESOLUTION_16);

    es8311_voice_volume_set(es_handle, volume, NULL);

    initialized = true;
}

void AudioManager::deinit() {
    if (!initialized) return;

    i2s.end();
    digitalWrite(PA, LOW);

    initialized = false;
}

void AudioManager::ampOn() {
    digitalWrite(PA, HIGH);
}

void AudioManager::ampOff() {
    digitalWrite(PA, LOW);
}

void AudioManager::setVolume(uint8_t vol) {
    if (vol > 100) vol = 100;
    volume = vol;

    if (es_handle) {
        es8311_voice_volume_set(es_handle, volume, NULL);
    }
}

void AudioManager::volumeUp(uint8_t step) {
    setVolume(volume + step);
}

void AudioManager::volumeDown(uint8_t step) {
    if (volume < step) volume = 0;
    else volume -= step;
    setVolume(volume);
}

uint8_t AudioManager::getVolume() {
    return volume;
}

void AudioManager::mute() {
    if (es_handle) {
        es8311_voice_volume_set(es_handle, 0, NULL);
    }
}

void AudioManager::unmute() {
    setVolume(volume);
}

void AudioManager::playTap() {
    //TODO: improve feedback sound (currently a bad click due to the short buffer and the I2S latency)
    if (!initialized) {
        init();
        if(!initialized) return;
    }

    const int samples = 200; // durata brevissima
    int16_t buffer[samples];

    for (int i = 0; i < samples; i++) {
        // click molto corto (decadimento)
        buffer[i] = (int16_t)(2000 * exp(-0.02 * i) * ((i % 2) ? 1 : -1));
    }

    i2s.write((uint8_t*)buffer, sizeof(buffer));
}