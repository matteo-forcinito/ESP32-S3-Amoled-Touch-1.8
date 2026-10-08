#include "services/sound.h"

#include "core/settings.h"
#include "core/sys.h"
#include "hardware/audio.h"
#include "services/radio_player.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <math.h>
#include <stdlib.h>

static const char *TAG = "sound";

#define RATE        22050
#define CHUNK       512

typedef enum
{
    SOUND_CLICK,
    SOUND_NOTIFICATION,
    SOUND_ALARM,
} sound_t;

typedef struct
{
    float freq;     /* 0 = silence */
    int ms;
} note_t;

static QueueHandle_t s_queue = NULL;
static volatile bool s_alarm = false;
static int16_t *s_buffer = NULL;

static bool speaker_free(void)
{
    return radio_player_state() == RADIO_STATE_IDLE || radio_player_state() == RADIO_STATE_FAILED;
}

/* Sine with a short fade in/out (no clicks), `level` 0..1. */
static void play_note(note_t note, float level)
{
    int total = RATE * note.ms / 1000;
    int fade = RATE / 200;   /* 5 ms */
    float phase = 0.0f;
    float step = 2.0f * (float)M_PI * note.freq / RATE;

    for (int done = 0; done < total;)
    {
        int n = total - done < CHUNK ? total - done : CHUNK;

        for (int i = 0; i < n; i++)
        {
            int pos = done + i;
            float env = 1.0f;

            if (pos < fade)
            {
                env = (float)pos / fade;
            }
            else if (pos > total - fade)
            {
                env = (float)(total - pos) / fade;
            }

            float value = note.freq > 0 ? sinf(phase) * env * level : 0.0f;
            phase += step;
            s_buffer[i] = (int16_t)(value * 24000.0f);
        }

        audio_write(s_buffer, (size_t)n);
        done += n;

        if (!s_alarm && note.ms > 400)
        {
            break;   /* alarm stopped in the middle of a long note */
        }
    }
}

static void play_sequence(const note_t *notes, int count, float level)
{
    for (int i = 0; i < count; i++)
    {
        play_note(notes[i], level);
    }
}

static void sound_task(void *arg)
{
    (void)arg;

    static const note_t click[] = {{2400, 12}};
    static const note_t chime[] = {{1319, 90}, {0, 30}, {1760, 160}};
    static const note_t melody[] = {{988, 140}, {0, 60}, {1319, 140}, {0, 60}, {1568, 220},
                                    {0, 120}, {1319, 140}, {0, 60}, {1568, 300}, {0, 700}};

    sound_t sound;

    while (xQueueReceive(s_queue, &sound, portMAX_DELAY) == pdTRUE)
    {
        /* The alarm MUST ring: give a stopping radio up to 10 s to free the speaker. */
        for (int i = 0; sound == SOUND_ALARM && !speaker_free() && i < 100; i++)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (sound == SOUND_ALARM && !s_alarm)
        {
            continue;   /* stopped while waiting */
        }

        if ((!speaker_free() && sound != SOUND_ALARM) || audio_start(RATE, 1) != ESP_OK)
        {
            continue;
        }

        int volume = settings_get()->volume;

        if (sound == SOUND_ALARM)
        {
            /* Start soft, reach full volume in ~20 s. */
            for (int round = 0; s_alarm; round++)
            {
                int vol = 40 + round * 6;
                audio_set_volume(vol > 100 ? 100 : vol);
                play_sequence(melody, sizeof(melody) / sizeof(melody[0]), 0.9f);
            }
        }
        else
        {
            audio_set_volume(volume < 20 ? 20 : volume);
            if (sound == SOUND_CLICK)
            {
                play_sequence(click, 1, 0.35f);
            }
            else
            {
                play_sequence(chime, 3, 0.6f);
            }

            vTaskDelay(pdMS_TO_TICKS(60));   /* let the DMA queue drain */
        }

        audio_stop();
    }
}

void sound_service_init(void)
{
    s_buffer = malloc(CHUNK * sizeof(int16_t));
    s_queue = xQueueCreate(4, sizeof(sound_t));

    if (s_buffer == NULL || s_queue == NULL ||
        !sys_task_create(sound_task, "sound", 3072, NULL, 5, 1))
    {
        ESP_LOGE(TAG, "Init failed");
    }
}

static void post(sound_t sound)
{
    if (s_queue != NULL)
    {
        xQueueSend(s_queue, &sound, 0);
    }
}

void sound_click(void)
{
    if (settings_get()->key_sounds)
    {
        post(SOUND_CLICK);
    }
}

void sound_notification(void)
{
    post(SOUND_NOTIFICATION);
}

void sound_alarm_start(void)
{
    if (!s_alarm)
    {
        s_alarm = true;
        post(SOUND_ALARM);
    }
}

void sound_alarm_stop(void)
{
    s_alarm = false;
}

bool sound_alarm_playing(void)
{
    return s_alarm;
}
