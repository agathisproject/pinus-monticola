// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasks.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#elif defined(__linux__)
#include "hw/platform_sim/state.h"
#endif

#include "agathis/base.h"
#include "agathis/comm.h"
#include "cli/cli.h"
#include "cmd.h"
#include "hw/platform.h"

static CLICmdDef_t p_cmd_root[3]  = {
    {"info", "[mfg]", "show module info", &cmd_Info},
    {"set",  "master", "change configuration values", &cmd_Set},
    {"cfg", "[show|save]", "show/save configuration", &cmd_Cfg},
};
static CLICmdFolder_t p_f_root = {"", sizeof(p_cmd_root) / sizeof(p_cmd_root[0]), p_cmd_root,
                                  NULL, NULL, NULL, NULL, NULL
                                 };

static CLICmdDef_t p_cmd_mod[5]  = {
    {"info", "", "show modules", &cmd_ModInfo},
    {"id", "", "identify module", &cmd_ModID},
    {"reset", "", "reset module", &cmd_ModReset},
    {"on", "", "power on module", &cmd_ModPowerOn},
    {"off", "", "power off module", &cmd_ModPowerOff},
};
static CLICmdFolder_t p_f_mod = {"mod", sizeof(p_cmd_mod) / sizeof(p_cmd_mod[0]), p_cmd_mod,
                                 &p_cmd_mod[0], NULL, NULL, NULL, NULL
                                };

static void p_CLIInit(void) {
    p_f_root.parent = &p_f_root;
    p_f_root.child = &p_f_mod;

    p_f_mod.parent = &p_f_root;

    cmd_SetPrompt();
    cli_Init(&p_f_root);

    gpio_SetRGB(0);
}

#if defined(ESP_PLATFORM)
void task_cli(void *pvParameter) {
    p_CLIInit();
    while (1) {
        //printf("%s", CLI_getPrompt());
        cli_Read();
        cli_Parse();
        cli_Execute();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
#elif defined(__linux__)
void *task_cli (void *vargp) {
    if ((SIM_STATE.sim_flags & SIM_FLAG_NO_CONSOLE) != 0) {
        while (1) {
            sleep(1);
        }
    } else {
        p_CLIInit();
        while (1) {
            //printf("%s", cli_GetPrompt());
            cli_Read();
            cli_Parse();
            cli_Execute();
        }
    }
}
#endif

#if defined(ESP_PLATFORM)
void task_rf(void *pvParameter) {
    //char *appName = pcTaskGetName(NULL);
    ag_comm_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    while (1) {
        ag_comm_main();
        ag_upd_remote_mods();
        ag_upd_alarm();
        ag_upd_hw();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
#elif defined(__linux__)
void *task_rf (void *vargp) {
    ag_comm_init();

    while (1) {
        ag_comm_main();
        ag_upd_remote_mods();
        ag_upd_alarm();
        ag_upd_hw();
        sleep(1);
    }
}
#endif
