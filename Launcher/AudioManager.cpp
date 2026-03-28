#include "AudioManager.h"
#include "pin_config.h"

extern I2SClass i2s;
static es8311_handle_t es_handle = NULL;

bool AudioManager::initialized = false;
uint8_t AudioManager::volume = 80;

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
    if (!initialized) return;

    const int samples = 200; // durata brevissima
    int16_t buffer[samples];

    for (int i = 0; i < samples; i++) {
        // click molto corto (decadimento)
        buffer[i] = (int16_t)(2000 * exp(-0.02 * i) * ((i % 2) ? 1 : -1));
    }

    i2s.write((uint8_t*)buffer, sizeof(buffer));
}