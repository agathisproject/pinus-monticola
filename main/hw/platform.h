// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PLATFORM_DR7WHAS4LTQNESQ3
#define PLATFORM_DR7WHAS4LTQNESQ3

#include <stdint.h>

#define APP_NAME "pinus-monitcola"

void pltf_Init(void);

void pltf_ShowHW(void);

void pltf_ShowSW(void);

typedef enum {
    PLTF_LOG_PATH,
} PltfParamId_t;

const char * pltf_GetParamStr(PltfParamId_t param_id);

/**
 * @brief returns the HW ID - usually the MAC address.
 * @param mac array of 6 * uint8_t
 */
void pltf_GetID(uint8_t *mac);

/**
 * @brief returns the HW ID - usually the MAC address.
 * @param mac array of 2 * uint32_t
 */
void pltf_GetIDCompact(uint32_t *mac);

void pltf_Reset(void);

void pltf_PwrOn(void);

void pltf_PwrOff(void);

/**
 * @brief send RBG code to LED. If the LED is not RBG capable it will just turn on.
 *
 * @param code uint32_t 0x00rrggbb where only the last 3 bytes are sent on the wire
 * byte 2 is the blue value
 * byte 1 is the green value
 * byte 0 is the red value
 */
void gpio_SetRGB(uint32_t code);

/**
 * @brief get platform temperature.
 */
float hw_GetTemperature(void);

#endif /* PLATFORM_DR7WHAS4LTQNESQ3 */
