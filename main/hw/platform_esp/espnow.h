// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ESPNOW_RSYL4WZS99DQRV9U
#define ESPNOW_RSYL4WZS99DQRV9U

#include <stdint.h>

#include "esp_now.h"

void espnow_init(void);

void espnow_set_tx_callback(void fptr(const uint8_t *mac_addr,
                                      esp_now_send_status_t status));

void espnow_set_rx_callback(void fptr(const uint8_t *mac_addr,
                                      const uint8_t *data, int len));

void espnow_add_peer(uint32_t mac_addr1, uint32_t mac_addr0);

void espnow_del_peer(uint32_t mac_addr1, uint32_t mac_addr0);

void espnow_tx(const uint8_t *mac_addr, const uint8_t *data, size_t len);

#endif /* ESPNOW_RSYL4WZS99DQRV9U */
