// SPDX-License-Identifier: GPL-3.0-or-later

#include "misc.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"

#define TAG "hw-misc"

void show_info(void) {
    uint8_t mac_addr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac_addr));
    ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", mac_addr[0],
             mac_addr[1], mac_addr[2], mac_addr[3],
             mac_addr[4], mac_addr[5], mac_addr[6], mac_addr[7]);
}

void nvs_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "non-volatile storage init done");
}
