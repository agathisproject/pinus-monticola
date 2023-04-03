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
#include "opt/utils.h"

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
    {"speed", "<id> [sec]", "speed test for module", &cmd_ModSpeed},
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

static AG_FRAME_L0 p_tx_frame = {{0, 0}, {0, 0}, {0}};
static AG_FRAME_L0 p_rx_frame = {{0, 0}, {0, 0}, {0}};

#define STATUS_DELAY 5000

static void p_send_status_frame(void) {
    agComm_InitTXFrame(&p_tx_frame);
    p_tx_frame.data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_STS, &p_tx_frame);
    p_tx_frame.data[4] = g_MCConfig.capsSW;
    agComm_SendFrame(&p_tx_frame);
}

#if defined(ESP_PLATFORM)
void task_rf(void *pvParameter) {
#elif defined(__linux__)
void *task_rf (void *vargp) {
#endif
    agComm_Init();
    usleep(100000);

    struct timeval t_now, t_status, t_update;

    p_send_status_frame();
    gettimeofday(&t_now, NULL);
    gettimeofday(&t_status, NULL);
    gettimeofday(&t_update, NULL);

    while (1) {
        gettimeofday(&t_now, NULL);
        if (diff_ms(t_now, t_status) > STATUS_DELAY) {
            p_send_status_frame();
            gettimeofday(&t_status, NULL);
        }

        if (agComm_GetRXState() != AG_RX_NONE) {
            if (agComm_GetRXFrameType() == AG_FRM_TYPE_STS) {
                agComm_CpRXFrame(&p_rx_frame);
                ag_AddRemoteMCInfo(p_rx_frame.src_mac, p_rx_frame.data[4]);
            } else if (agComm_GetRXFrameType() == AG_FRM_TYPE_CMD) {
                if (agComm_IsRXFrameFromMaster()) {
                    switch (agComm_GetRXFrameCmd()) {
                        case AG_FRM_CMD_ID: {
                            agComm_CpRXFrame(&p_rx_frame);
                            ag_LEDCtrl(AG_LED_BLINK);
                            break;
                        }
                        case AG_FRM_CMD_RESET: {
                            agComm_CpRXFrame(&p_rx_frame);
                            pltf_Reset();
                            break;
                        }
                        case AG_FRM_CMD_POWER_OFF: {
                            agComm_CpRXFrame(&p_rx_frame);
                            pltf_PwrOff();
                            break;
                        }
                        case AG_FRM_CMD_POWER_ON: {
                            agComm_CpRXFrame(&p_rx_frame);
                            pltf_PwrOn();
                            break;
                        }
                        case AG_FRM_CMD_PING: {
                            agComm_CpRXFrame(&p_rx_frame);
                            agComm_InitTXFrame(&p_tx_frame);
                            p_tx_frame.dst_mac[0] = p_rx_frame.src_mac[0];
                            p_tx_frame.dst_mac[1] = p_rx_frame.src_mac[1];

                            p_tx_frame.data[0] = AG_FRM_PROTO_VER1;
                            agComm_SetFrameType(AG_FRM_TYPE_ACK, &p_tx_frame);
                            agComm_SetFrameCmd(AG_FRM_CMD_PING, &p_tx_frame);
                            agComm_SendFrame(&p_tx_frame);
                            break;
                        }
                        default: {
                            printf("W (%s) UNRECOGNIZED command %d\n", __func__, agComm_GetRXFrameCmd());
                            break;
                        }
                    }
                }
            }
        }

        if (diff_ms(t_now, t_update) > 1000) {
            ag_UpdAlarm();
            ag_UpdHWState();
            ag_UpdRemoteMCs();
            gettimeofday(&t_update, NULL);
        }
        usleep(10000);
    }
#if defined(ESP_PLATFORM)
    vTaskDelete(NULL);
#endif
}
