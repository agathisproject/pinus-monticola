// SPDX-License-Identifier: GPL-3.0-or-later

#include "espnow.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_crc.h"

#include "base.h"

#define TAG "hw-espnow"

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

static void p_wifi_init(void) {
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF,
                                           WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR) );
#endif
    ESP_LOGI(TAG, "wifi init done");
}

//static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void example_espnow_send_cb(const uint8_t *mac_addr,
                                   esp_now_send_status_t status) {
    ESP_LOGI(TAG, "TX to "MACSTR" status %d", MAC2STR(mac_addr), status);
}

static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {
    ESP_LOGI(TAG, "RX from "MACSTR" %d B", MAC2STR(mac_addr), len);
//    ESP_LOGI(TAG, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", data[0], data[1], data[2], data[3], data[4],
//             data[5], data[6], data[7], data[8], data[9]);
}

void espnow_add_peer(uint32_t mac_addr1, uint32_t mac_addr0) {
    //ESP_LOGI(TAG, "add peer %x %x\n", (unsigned int) mac_addr1, (unsigned int) mac_addr0);
    uint8_t mac_addr[6] = {(uint8_t) (mac_addr1 >> 16), (uint8_t) (mac_addr1 >> 8), (uint8_t) (mac_addr1),
                           (uint8_t) (mac_addr0 >> 16), (uint8_t) (mac_addr0 >> 8), (uint8_t) (mac_addr0)
                        };
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "CANNOT malloc peer");
        esp_now_deinit();
        return;
    }

    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);
}

void espnow_del_peer(uint32_t mac_addr1, uint32_t mac_addr0) {
    //ESP_LOGI(TAG, "del peer %x %x\n", (unsigned int) mac_addr1, (unsigned int) mac_addr0);
    uint8_t mac_addr[6] = {(uint8_t) (mac_addr1 >> 16), (uint8_t) (mac_addr1 >> 8), (uint8_t) (mac_addr1),
                           (uint8_t) (mac_addr0 >> 16), (uint8_t) (mac_addr0 >> 8), (uint8_t) (mac_addr0)
    };
    ESP_ERROR_CHECK( esp_now_del_peer(mac_addr) );
}

void espnow_init(void) {
    p_wifi_init();

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    espnow_add_peer(0x00FFFFFF, 0x00FFFFFF);
}

void espnow_set_tx_callback(void fptr(const uint8_t *mac_addr, esp_now_send_status_t status)) {
    ESP_ERROR_CHECK( esp_now_register_send_cb(fptr) );
}

void espnow_set_rx_callback(void fptr(const uint8_t *mac_addr, const uint8_t *data, int len)) {
    ESP_ERROR_CHECK( esp_now_register_recv_cb(fptr) );
}

void espnow_tx(const uint8_t *mac_addr, const uint8_t *data, size_t len) {
    if(esp_now_send(mac_addr, data, len) != ESP_OK) {
        ESP_LOGE(TAG, "TX error");
    }
}
