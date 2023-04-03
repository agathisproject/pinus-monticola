// SPDX-License-Identifier: GPL-3.0-or-later

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "tasks.h"
#include "agathis/base.h"
#include "hw/platform.h"

static void p_banner() {
    ESP_LOGI(APP_NAME, "start");
}

void app_main(void) {
    pltf_Init();
    p_banner();
    ag_Init();

    xTaskCreate(task_cli, "task_CLI", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(task_rf, "task_RF", 4096, NULL, (tskIDLE_PRIORITY + 2), NULL);
}
