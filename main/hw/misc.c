// SPDX-License-Identifier: GPL-3.0-or-later

#include "misc.h"

#if defined(ESP_PLATFORM)
#include "esp_mac.h"
#elif defined(__linux__)
#include "../sim/state.h"
#endif

/**
 * @brief returns the HW ID - usually the MAC address
 * @param mac array of 6 * uint8_t
 */
void get_HW_ID(uint8_t *mac) {
#if defined(ESP_PLATFORM)
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buff));
    for (int i = 0; i < 6; i++) {
        mac[0] = buff[5 - i];
    }
#elif defined(__linux__)
    for (int i = 0; i < 6; i++) {
        mac[i] = SIM_STATE.mac[i];
    }
#endif
}

/**
 * @brief returns the HW ID - usually the MAC address
 * @param mac array of 2 * uint32_t
 */
void get_HW_ID_compact(uint32_t *mac) {
#if defined(ESP_PLATFORM)
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buff));
    mac[1] = ((uint32_t) buff[0] << 16) | ((uint32_t) buff[1] << 8) | buff[2];
    mac[0] = ((uint32_t) buff[3] << 16) | ((uint32_t) buff[4] << 8) | buff[5];
#elif defined(__linux__)
    // *INDENT-OFF*
    mac[1] = ((uint32_t) SIM_STATE.mac[5] << 16) | ((uint32_t) SIM_STATE.mac[4] << 8) | SIM_STATE.mac[3];
    mac[0] = ((uint32_t) SIM_STATE.mac[2] << 16) | ((uint32_t) SIM_STATE.mac[1] << 8) | SIM_STATE.mac[0];
    // *INDENT-ON*
#endif
}
