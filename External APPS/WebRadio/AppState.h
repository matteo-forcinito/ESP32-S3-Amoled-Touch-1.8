#ifndef APP_STATE_H
#define APP_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_NONE,
    APP_HOME,
    APP_BTREMOTE,
    APP_WIFISTATUS,
    APP_USBREMOTE,
    APP_SETTINGS,
    APP_FLASH,
    APP_SETTIME,
    APP_INSTALLER,
    APP_CONTROL_CENTER,
    APP_INVALID_APP
} AppState;

#ifdef __cplusplus
}
#endif

#endif // APP_STATE_H
