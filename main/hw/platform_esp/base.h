// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASE_HDTCR4RCYNWCHYS4
#define BASE_HDTCR4RCYNWCHYS4

#include <stdint.h>

void nvs_init(void);

void gpio_init(void);

/**
 * @brief send RBG code to LED
 *
 * @param code uint32_t 0x00rrggbb where only the last 3 bytes are sent on the wire
 * byte 2 is the blue value
 * byte 1 is the green value
 * byte 0 is the red value
 */
void gpio_RGB_send(uint32_t code);

#endif /* BASE_HDTCR4RCYNWCHYS4 */
