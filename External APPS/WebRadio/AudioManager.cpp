#include "AudioManager.h"
#include "pin_config.h"

extern I2SClass i2s;

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
        Serial.println("I2S init failed");
        return;
    }

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