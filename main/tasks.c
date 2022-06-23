// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasks.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"

#include "hw/espnow.h"

void task_cli(void *pvParameter) {
    while (1) {
//        char *appName = pcTaskGetName(NULL);
//        ESP_LOGI(appName, "");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static void p_espnow_tx_cbk(const uint8_t *mac_addr,
                            esp_now_send_status_t status) {
    char *appName = pcTaskGetName(NULL);
    ESP_LOGI(appName, "TX to "MACSTR" status %d", MAC2STR(mac_addr), status);
}

static void p_espnow_rx_cbk(const uint8_t *mac_addr, const uint8_t *data, int len) {
    char *appName = pcTaskGetName(NULL);
    ESP_LOGI(appName, "RX from "MACSTR" %d B", MAC2STR(mac_addr), len);
}

void task_rf(void *pvParameter) {
    //char *appName = pcTaskGetName(NULL);
    espnow_init();
    espnow_set_tx_callback(p_espnow_tx_cbk);
    espnow_set_rx_callback(p_espnow_rx_cbk);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (1) {
        espnow_tx(0, NULL, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
