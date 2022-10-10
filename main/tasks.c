// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasks.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hw/esp_platform.h"
#endif

#include "agathis/comm.h"
#include "cli/cli.h"
#include "hw/misc.h"

static void p_CLI_init_prompt(void) {
    char prompt[CLI_PROMPT_SIZE];
    uint32_t mac[2];

    get_HW_ID_compact(mac);
    snprintf(prompt, CLI_PROMPT_SIZE, "[%06lx:%06lx]$ ", mac[1], mac[0]);
    printf("press ? for help\n");
    CLI_setPrompt(prompt);

    gpio_RGB_send(0);
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
#elif defined(ESP_PLATFORM)
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

#if defined(__linux__)
void *task_rf (void *vargp) {
    ag_comm_init();

    while (1) {
        ag_comm_main();
    }
}
#elif defined(ESP_PLATFORM)
void task_rf(void *pvParameter) {
    //char *appName = pcTaskGetName(NULL);
    ag_comm_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (1) {
        ag_comm_main();
    }
    vTaskDelete(NULL);
}
#endif
