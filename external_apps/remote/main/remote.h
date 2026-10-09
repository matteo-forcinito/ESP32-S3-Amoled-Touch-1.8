#ifndef REMOTE_H
#define REMOTE_H

/*
 * Remote Control: shared pieces of the app.
 *
 *   remote_home.c     home screen: connection, control modes, options
 *   keyboard_app.c    the swipe keyboard
 *   swipe.c           swipe decoder (pure C, host-tested)
 *   main.c            start-up
 */

#include "core/app.h"
#include "hid_link/hid_link.h"
#include "swipe.h"

#include <stdbool.h>

typedef enum
{
    DICT_IT,
    DICT_EN,
} remote_dict_t;

typedef struct
{
    uint8_t transport;      /* hid_link_transport_t */
    uint8_t host_layout;    /* hid_host_layout_t */
    uint8_t dictionary;     /* remote_dict_t */
    bool swipe;             /* swipe typing on */
    bool auto_caps;         /* capital letter at the start of a sentence */
} remote_settings_t;

/* Settings kept in NVS ("remote" namespace). */
const remote_settings_t *remote_settings(void);
void remote_settings_save(const remote_settings_t *settings);
void remote_settings_load(void);

/* Dictionary for the current language (loaded once, NULL if out of memory). */
const swipe_dict_t *remote_dictionary(void);
void remote_dictionary_select(remote_dict_t dict);

/* Start the saved transport (at boot and after a change). */
void remote_connect(void);

/* "Bluetooth: in attesa" ... for headers. */
const char *remote_status_text(void);

extern const app_t keyboard_app;
extern const app_t remote_options_app;

void remote_home_create(lv_obj_t *screen);

#endif
