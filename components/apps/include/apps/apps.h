#ifndef APPS_APPS_H
#define APPS_APPS_H

/*
 * The built-in apps and the watch shell (home screen).
 *
 *                     ┌──────────────┐
 *                     │ control      │   swipe down from the watch face
 *                     │ center       │
 *     ┌────────────┐  ├──────────────┤  ┌────────────┐
 *     │ now playing│◄─│  WATCH FACE  │─►│  app list  │
 *     └────────────┘  ├──────────────┤  └────────────┘
 *                     │ notifications│   swipe up
 *                     └──────────────┘
 *
 * To add an app: write my_app.c with a `const app_t my_app`, declare it in
 * apps_internal.h and add it to the list in apps.c. That is all.
 */

/* Register every built-in app and build the home screen. LVGL lock held. */
void apps_init(void);

#endif
