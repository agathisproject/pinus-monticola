// SPDX-License-Identifier: GPL-3.0-or-later

#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "agathis/base.h"
#include "agathis/comm.h"
#include "agathis/config.h"
#include "cli/cli.h"
#include "hw/storage.h"
#include "hw/platform.h"

void cmd_SetPrompt(void) {
    char prompt[CLI_PROMPT_MAX_LEN];
    uint32_t mac[2];

    hw_GetIDCompact(mac);
#if defined(ESP_PLATFORM)
    snprintf(prompt, CLI_PROMPT_MAX_LEN, "[%06lx:%06lx]%c ", mac[1], mac[0],
             ((g_MCConfig.capsSW && AG_CAP_SW_TMC) ? '#' : '$'));
#elif defined(__linux__)
    snprintf(prompt, CLI_PROMPT_MAX_LEN, "[%06x:%06x]%c ", mac[1], mac[0],
             ((g_MCConfig.capsSW && AG_CAP_SW_TMC) ? '#' : '$'));
#endif
    cli_SetPrompt(prompt);
}

CLIStatus_t cmd_Info(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk == 1) {
        printf("error: %d\n", g_MCState.lastErr);
#if defined(ESP_PLATFORM)
        printf("TX/RX pkts.: %lu/%lu\n", g_MCStats.txCnt, g_MCStats.rxCnt);
#elif defined(__linux__)
        printf("TX/RX pkts.: %u/%u\n", g_MCStats.txCnt, g_MCStats.rxCnt);
#endif
        printf("temp: %.2f C\n", hw_GetTemperature());
        return CLI_NO_ERROR;
    } else if (cmdp->nTk == 2) {
        if (strncmp(cmdp->tokens[1], "hw", 2) == 0) {
            platform_Show();
        } else if (strncmp(cmdp->tokens[1], "sw", 2) == 0) {
            printf("APP_NAME: %s\n", APP_NAME);
            printf("APP_VER: %d\n", 0);
        } else {
            return CLI_CMD_PARAMS_ERROR;
        }
        return CLI_NO_ERROR;
    }
    return CLI_CMD_PARAMS_ERROR;
}

CLIStatus_t cmd_Set(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 3) {
        return CLI_CMD_PARAMS_ERROR;
    }

    if (strncmp(cmdp->tokens[1], "master", 6) == 0) {
        if (strncmp(cmdp->tokens[2], "on", 2) == 0) {
            g_MCConfig.capsSW |= AG_CAP_SW_TMC;
        } else {
            g_MCConfig.capsSW &= (uint8_t) (~AG_CAP_SW_TMC);
        }
        cmd_SetPrompt();
        return CLI_NO_ERROR;
    }
    return CLI_CMD_PARAMS_ERROR;
}

CLIStatus_t cmd_Cfg(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    if (strncmp(cmdp->tokens[1], "show", 4) == 0) {
        printf("caps = %#04x/%#04x\n", g_MCConfig.capsHW, g_MCConfig.capsSW);
        printf("MFR_NAME = %s\n", g_MCConfig.mfrName);
        printf("MFR_PN = %s\n", g_MCConfig.mfrPN);
        printf("MFR_SN = %s\n", g_MCConfig.mfrSN);
        return CLI_NO_ERROR;
    } else if (strncmp(cmdp->tokens[1], "save", 4) == 0) {
        stor_SaveState();
        return CLI_NO_ERROR;
    } else if (strncmp(cmdp->tokens[1], "erase", 4) == 0) {
        stor_EraseState();
        return CLI_NO_ERROR;
    }
    return CLI_CMD_PARAMS_ERROR;
}

CLIStatus_t cmd_ModInfo(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk > 1) {
        return CLI_CMD_PARAMS_ERROR;
    }

    for (int i = 0; i < AG_MC_MAX_CNT; i ++) {
        if (g_RemoteMCs[i].lastSeen == -1) {
            continue;
        }
        if ((g_RemoteMCs[i].capsSW & AG_CAP_SW_TMC) != 0) {
            printf("%2d - %06x:%06x M (%ds)\n", i,
                   (unsigned int) g_RemoteMCs[i].mac[1], (unsigned int) g_RemoteMCs[i].mac[0],
                   g_RemoteMCs[i].lastSeen);
        } else  {
            printf("%2d - %06x:%06x (%ds)\n", i,
                   (unsigned int) g_RemoteMCs[i].mac[1], (unsigned int) g_RemoteMCs[i].mac[0],
                   g_RemoteMCs[i].lastSeen);
        }
    }
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModLed(CLICmdParsed_t *cmdp) {
    if ((g_MCConfig.capsSW & AG_CAP_SW_TMC) == 0) {
        printf("E (%d) ONLY master can do this\n", __LINE__);
        return CLI_CMD_FAIL;
    }
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (g_RemoteMCs[mc_id].lastSeen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = g_RemoteMCs[mc_id].mac[1];
    frame->dst_mac[0] = g_RemoteMCs[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_ID, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModReset(CLICmdParsed_t *cmdp) {
    if ((g_MCConfig.capsSW & AG_CAP_SW_TMC) == 0) {
        printf("E (%d) ONLY master can do this\n", __LINE__);
        return CLI_CMD_FAIL;
    }
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (g_RemoteMCs[mc_id].lastSeen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = g_RemoteMCs[mc_id].mac[1];
    frame->dst_mac[0] = g_RemoteMCs[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_RESET, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModPowerOn(CLICmdParsed_t *cmdp) {
    if ((g_MCConfig.capsSW & AG_CAP_SW_TMC) == 0) {
        printf("E (%d) ONLY master can do this\n", __LINE__);
        return CLI_CMD_FAIL;
    }
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (g_RemoteMCs[mc_id].lastSeen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = g_RemoteMCs[mc_id].mac[1];
    frame->dst_mac[0] = g_RemoteMCs[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_POWER_ON, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModPowerOff(CLICmdParsed_t *cmdp) {
    if ((g_MCConfig.capsSW & AG_CAP_SW_TMC) == 0) {
        printf("E (%d) ONLY master can do this\n", __LINE__);
        return CLI_CMD_FAIL;
    }
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (g_RemoteMCs[mc_id].lastSeen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = g_RemoteMCs[mc_id].mac[1];
    frame->dst_mac[0] = g_RemoteMCs[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_POWER_OFF, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModSpeed(CLICmdParsed_t *cmdp) {
    if ((g_MCConfig.capsSW & AG_CAP_SW_TMC) == 0) {
        printf("E (%d) ONLY master can do this\n", __LINE__);
        return CLI_CMD_FAIL;
    }
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (g_RemoteMCs[mc_id].lastSeen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame;

    uint16_t n_ping = 0, n_reply = 0;
    clock_t t0 = clock();
    while ((clock() - t0) < CLOCKS_PER_SEC) {
        frame = agComm_GetTXFrame();
        if (frame == NULL) {
            printf("%s - CANNOT get TX frame\n", __func__);
            continue;
        }
        frame->dst_mac[1] = g_RemoteMCs[mc_id].mac[1];
        frame->dst_mac[0] = g_RemoteMCs[mc_id].mac[0];
        frame->data[0] = AG_FRM_PROTO_VER1;
        agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
        agComm_SetFrameCmd(AG_FRM_CMD_PING, frame);
        agComm_SetFrameRTS(frame);
        n_ping ++;

        frame = agComm_GetRXFrame();
        if (frame == NULL) {
            printf("%s - CANNOT get RX frame\n", __func__);
            continue;
        }
        agComm_SetFrameDone(frame);
        n_reply ++;
    }
    printf("TX packets: %d, RX packets: %d\n", n_ping, n_reply);
    return CLI_NO_ERROR;
}
