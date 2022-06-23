// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GPIO_HDTCR4RCYNWCHYS4
#define GPIO_HDTCR4RCYNWCHYS4

#include <stdint.h>

void gpio_init(void);

/**
 * @brief send RBG code to LED
 *
 * @param code uint32_t 0x00aabbcc where only the last 3 bytes are sent on the wire
 */
void gpio_RGB_send(uint32_t code);

#endif /* GPIO_HDTCR4RCYNWCHYS4 */
