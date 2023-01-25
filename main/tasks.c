// SPDX-License-Identifier: GPL-3.0-or-later

#include "tasks.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
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
    {"info", "[hw|sw]", "show module info", &cmd_Info},
    {"set",  "master [on|off]", "change configuration values", &cmd_Set},
    {"cfg", "[show|save|erase]", "show/save configuration", &cmd_Cfg},
};
static CLICmdFolder_t p_f_root = {"", sizeof(p_cmd_root) / sizeof(p_cmd_root[0]), p_cmd_root,
                                  NULL, NULL, NULL, NULL, NULL
                                 };

static CLICmdDef_t p_cmd_mod[6]  = {
    {"info", "", "show modules", &cmd_ModInfo},
    {"led", "<id>", "blink LED on module", &cmd_ModLed},
    {"reset", "<id>", "reset module", &cmd_ModReset},
    {"on", "<id>", "power on module", &cmd_ModPowerOn},
    {"off", "<id>", "power off module", &cmd_ModPowerOff},
    {"speed", "<id>", "speed test for module", &cmd_ModSpeed},
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
        vTaskDelay(10 / portTICK_PERIOD_MS);
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
    agComm_Init();
    vTaskDelay(100 / portTICK_PERIOD_MS);

    agComm_SendStatus();
    time_t t_status = time(NULL);
    time_t t_update = time(NULL);

    while (1) {
        time_t t_now = time(NULL);
        if ((t_now - t_status) > 4) {
            agComm_SendStatus();
            t_status = time(NULL);
        }
        agComm_SendFrame();
        agComm_RecvFrame();

        ag_UpdAlarm();
        ag_UpdHWState();
        if ((t_now - t_update) > 0) {
            ag_UpdRemoteMCs();
            t_update = t_now;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
#elif defined(__linux__)
void *task_rf (void *vargp) {
    agComm_Init();

    agComm_SendStatus();
    time_t t_status = time(NULL);
    time_t t_update = time(NULL);

    while (1) {
        time_t t_now = time(NULL);
        if ((t_now - t_status) > 4) {
            agComm_SendStatus();
            t_status = time(NULL);
        }
        agComm_SendFrame();
        agComm_RecvFrame();

        ag_UpdAlarm();
        ag_UpdHWState();
        if ((t_now - t_update) > 0) {
            ag_UpdRemoteMCs();
            t_update = t_now;
        }
        usleep(10000);
    }
}
#endif
