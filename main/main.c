// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "tasks.h"
#include "agathis/base.h"
#include "hw/platform.h"

void app_main(void) {
    char *appName = pcTaskGetName(NULL);
    ESP_LOGI(appName, "start");

    platform_Init();
    ag_Init();

    xTaskCreate(task_cli, "task_CLI", 2048, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(task_rf, "task_RF", 4096, NULL, (tskIDLE_PRIORITY + 2), NULL);

//    vTaskDelay(10 / portTICK_PERIOD_MS);
//    while (1) {
//        gpio_SetRGB(0x00000000);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        gpio_SetRGB(0x0000FF00);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        gpio_SetRGB(0x000000FF);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        gpio_SetRGB(0x00FF0000);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//    }
}
