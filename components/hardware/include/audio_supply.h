#ifndef AUDIO_SUPPLY_H
#define AUDIO_SUPPLY_H
#include "esp_err.h"
/* Board-local A3V3 rail, shared by codec analogue supply and microphone. */
esp_err_t audio_supply_acquire(void);
void audio_supply_release(void);
#endif
