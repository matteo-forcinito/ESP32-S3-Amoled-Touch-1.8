#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*
 * Picks the pin file of the board chosen in menuconfig
 * (Board (hardware layer) -> Board model).
 *
 * To support another Waveshare board: copy waveshare_amoled_1_8.h, change
 * the pins and the BOARD_HAS_* flags, add it to the Kconfig choice and here.
 * Drivers for chips the board does not have are simply never started.
 */

#include "sdkconfig.h"

#if defined(CONFIG_BOARD_WS_AMOLED_1_8)
#include "waveshare_amoled_1_8.h"
#else
#error "No board selected in menuconfig"
#endif

#endif
