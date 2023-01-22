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
             ((MOD_STATE.caps_sw && AG_CAP_SW_TMC) ? '#' : '$'));
#elif defined(__linux__)
    snprintf(prompt, CLI_PROMPT_MAX_LEN, "[%06x:%06x]%c ", mac[1], mac[0],
             ((MOD_STATE.caps_sw && AG_CAP_SW_TMC) ? '#' : '$'));
#endif
    cli_SetPrompt(prompt);
}

CLIStatus_t cmd_Info(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk == 1) {
        printf("error = %d\n", MOD_STATE.last_err);
        printf("temp = %.2f C\n", hw_GetTemperature());
        return CLI_NO_ERROR;
    } else if (cmdp->nTk == 2) {
        if (strncmp(cmdp->tokens[1], "mfg", 3) == 0) {
            printf("MFR_NAME = %s\n", MOD_STATE.mfr_name);
            printf("MFR_PN = %s\n", MOD_STATE.mfr_pn);
            printf("MFR_SN = %s\n", MOD_STATE.mfr_sn);
        } else {
            return CLI_CMD_PARAMS_ERROR;
        }
        return CLI_NO_ERROR;
    }
    return CLI_CMD_PARAMS_ERROR;
}

CLIStatus_t cmd_Stats(CLICmdParsed_t *cmdp) {
    printf("TX/RX pkts.: %lu/%lu\n", MOD_STATS.tx_cnt, MOD_STATS.rx_cnt);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_Set(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 3) {
        return CLI_CMD_PARAMS_ERROR;
    }

    if (strncmp(cmdp->tokens[1], "master", 6) == 0) {
        if (strncmp(cmdp->tokens[2], "on", 2) == 0) {
            MOD_STATE.caps_sw |= AG_CAP_SW_TMC;
        } else {
            MOD_STATE.caps_sw &= (uint8_t) (~AG_CAP_SW_TMC);
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
        printf("caps = %#04x/%#04x/%#04x\n", MOD_STATE.caps_hw_ext,
               MOD_STATE.caps_hw_int, MOD_STATE.caps_sw);
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
        if (REMOTE_MODS[i].last_seen == -1) {
            continue;
        }
        if ((REMOTE_MODS[i].caps & AG_CAP_SW_TMC) != 0) {
            printf("%2d - %06x:%06x M (%ds)\n", i,
                   (unsigned int) REMOTE_MODS[i].mac[1], (unsigned int) REMOTE_MODS[i].mac[0],
                   REMOTE_MODS[i].last_seen);
        } else  {
            printf("%2d - %06x:%06x (%ds)\n", i,
                   (unsigned int) REMOTE_MODS[i].mac[1], (unsigned int) REMOTE_MODS[i].mac[0],
                   REMOTE_MODS[i].last_seen);
        }
    }
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModLed(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_ID, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModReset(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_RESET, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModPowerOn(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_POWER_ON, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModPowerOff(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CLI_NO_ERROR;
    }

    AG_FRAME_L0 *frame = agComm_GetTXFrame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CLI_NO_ERROR;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_FRM_PROTO_VER1;
    agComm_SetFrameType(AG_FRM_TYPE_CMD, frame);
    agComm_SetFrameCmd(AG_FRM_CMD_POWER_OFF, frame);
    agComm_SetFrameRTS(frame);
    return CLI_NO_ERROR;
}

CLIStatus_t cmd_ModSpeed(CLICmdParsed_t *cmdp) {
    if (cmdp->nTk != 2) {
        return CLI_CMD_PARAMS_ERROR;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->tokens[1], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CLI_NO_ERROR;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
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
        frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
        frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
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
