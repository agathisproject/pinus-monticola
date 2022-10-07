// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasks.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"

//#include "agathis/comm.h"
#include "cli/cli.h"
#include "hw/espnow.h"

static void p_CLI_init_prompt(void) {
    char prompt[CLI_PROMPT_SIZE];
    uint32_t mac[2] = {0x00445566, 0x00112233};

//    stor_get_MAC_compact(mac);
    snprintf(prompt, CLI_PROMPT_SIZE, "[%06lx:%06lx]$ ", mac[1], mac[0]);
    printf("press ? for help\n");
    CLI_setPrompt(prompt);
}

#if defined(__linux__)
void *task_cli (void *vargp) {
    uint8_t parseSts = 1;

    if ((SIM_STATE.sim_flags & SIM_FLAG_NO_CONSOLE) != 0) {
        while (1) {
            sleep(1);
        }
    } else {
        p_CLI_init_prompt();
        while (1) {
            printf("%s", CLI_getPrompt());
            CLI_getCmd();
            parseSts = CLI_parseCmd();
            if (parseSts == 0) {
                CLI_execute();
            }
        }
    }
}
#elif defined (CONFIG_IDF_TARGET)
void task_cli(void *pvParameter) {
    uint8_t parseSts;

    p_CLI_init_prompt();
    while (1) {
        printf("%s", CLI_getPrompt());
        CLI_getCmd();
        parseSts = CLI_parseCmd();
        if (parseSts == 0) {
            CLI_execute();
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
#endif

static void p_espnow_tx_cbk(const uint8_t *mac_addr,
                            esp_now_send_status_t status) {
    char *appName = pcTaskGetName(NULL);
    ESP_LOGI(appName, "TX to "MACSTR" status %d", MAC2STR(mac_addr), status);
}

static void p_espnow_rx_cbk(const uint8_t *mac_addr, const uint8_t *data, int len) {
    char *appName = pcTaskGetName(NULL);
    ESP_LOGI(appName, "RX from "MACSTR" %d B", MAC2STR(mac_addr), len);
}

#if defined(__linux__)
void *task_rf (void *vargp) {
    ag_comm_init();

    while (1) {
        ag_comm_main();
    }
}
#elif defined (CONFIG_IDF_TARGET)
void task_rf(void *pvParameter) {
    //char *appName = pcTaskGetName(NULL);
    espnow_init();
    espnow_set_tx_callback(p_espnow_tx_cbk);
    espnow_set_rx_callback(p_espnow_rx_cbk);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (1) {
        //espnow_tx(0, NULL, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
#endif
