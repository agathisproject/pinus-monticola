// SPDX-License-Identifier: GPL-3.0-or-later

#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "driver/rmt_tx.h"
#include "driver/temperature_sensor.h"

#include "../platform.h"

#define TAG "hw-esp"

static void p_nvs_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "non-volatile storage init done");
}

static rmt_channel_handle_t led_ch_hndl = NULL;
static rmt_tx_channel_config_t led_ch_config = {
    .clk_src = RMT_CLK_SRC_DEFAULT,     // select source clock
    .gpio_num = CONFIG_LED_PIN_NO,
    .mem_block_symbols = 64,            // memory block size, 64 * 4 = 256Bytes
    .resolution_hz = 10 * 1000 * 1000,  // 10MHz tick resolution, i.e. 1 tick = 0.1us
    .trans_queue_depth = 4,             // set the number of transactions that can be pending in the background
    .flags.invert_out = false,          // don't invert output signal
    .flags.with_dma = false,            // don't need DMA backend
};
static rmt_encoder_handle_t led_enc_hndl = NULL;
static rmt_transmit_config_t led_tx_config = {
    .loop_count = 0,                    // no transfer loop
};
#if CONFIG_LED_RGB_PART_NONE
static rmt_bytes_encoder_config_t bytes_encoder_config = NULL;
#elif CONFIG_LED_RGB_PART_SK68
static rmt_bytes_encoder_config_t bytes_encoder_config = {
    .bit0 = {
        .level0 = 1,
        .duration0 = 4, // T0H in us
        .level1 = 0,
        .duration1 = 8, // T0L in us
    },
    .bit1 = {
        .level0 = 1,
        .duration0 = 8, // T1H in us
        .level1 = 0,
        .duration1 = 4, // T1L in us
    },
    .flags.msb_first = 1
};
#endif

static void p_RGB_init(void) {
    ESP_ERROR_CHECK(rmt_new_tx_channel(&led_ch_config, &led_ch_hndl));
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &led_enc_hndl));
    ESP_ERROR_CHECK(rmt_enable(led_ch_hndl));
}

static void p_gpio_init(void) {
    p_RGB_init();
    ESP_LOGI(TAG, "GPIO init done");
}

static temperature_sensor_handle_t temp_handle = NULL;
static temperature_sensor_config_t temp_sensor = {
    .range_min = 0,
    .range_max = 70,
};

static void p_temp_init(void) {
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor, &temp_handle));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
    ESP_LOGI(TAG, "temperature sensor init done");
}

void platform_init(void) {
    p_nvs_init();
    p_gpio_init();
    p_temp_init();
}

void hw_GetID(uint8_t *mac) {
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buff));
    for (int i = 0; i < 6; i++) {
        mac[0] = buff[5 - i];
    }
}

void hw_GetIDCompact(uint32_t *mac) {
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buff));
    mac[1] = ((uint32_t) buff[0] << 16) | ((uint32_t) buff[1] << 8) | buff[2];
    mac[0] = ((uint32_t) buff[3] << 16) | ((uint32_t) buff[4] << 8) | buff[5];
}

void gpio_SetRGB(uint32_t code) {
#if CONFIG_LED_RGB_PART_SK68
    //ESP_LOGI(TAG, "%#010x", code);
    uint8_t data[3] = {(uint8_t) ((code >> 8) & 0xFF), (uint8_t) ((code >> 16) & 0xFF), (uint8_t) (code & 0xFF)};
    ESP_ERROR_CHECK(rmt_transmit(led_ch_hndl, led_enc_hndl, data, 3,
                                 &led_tx_config));
#endif
}

float hw_GetTemperature(void) {
    float tmp;

    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &tmp));
    return tmp;
}
