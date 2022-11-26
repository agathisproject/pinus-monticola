// SPDX-License-Identifier: GPL-3.0-or-later

#include "cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../agathis/base.h"
#include "../agathis/comm.h"
#include "../agathis/config.h"
#include "../hw/storage.h"

CLI_CMD_RETURN_t cmd_info(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 0) {
        return CMD_WRONG_N;
    }

    printf("caps = %#04x/%#04x/%#04x\n", MOD_STATE.caps_hw_ext,
           MOD_STATE.caps_hw_int, MOD_STATE.caps_sw);
    printf("error = %d\n", MOD_STATE.last_err);
    printf("MFR_NAME = %s\n", MOD_STATE.mfr_name);
    printf("MFR_PN = %s\n", MOD_STATE.mfr_pn);
    printf("MFR_SN = %s\n", MOD_STATE.mfr_sn);
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_set(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 2) {
        return CMD_WRONG_N;
    }

    if (strncmp(cmdp->params[0], "master", 6) == 0) {
        if (strncmp(cmdp->params[1], "on", 2) == 0) {
            MOD_STATE.caps_sw |= AG_CAP_SW_TMC;
        } else {
            MOD_STATE.caps_sw &= (uint8_t) (~AG_CAP_SW_TMC);
        }
    }

    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_save(CLI_PARSED_CMD_t *cmdp) {
#if MOD_HAS_STORAGE
    stor_save_state();
#endif
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_mod_info(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 0) {
        return CMD_WRONG_N;
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
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_mod_id(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 1) {
        return CMD_WRONG_N;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->params[0], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CMD_DONE;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CMD_DONE;
    }

    AG_FRAME_L0 *frame = ag_comm_get_tx_frame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CMD_DONE;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_PROTO_VER1;
    frame->data[1] = AG_PKT_TYPE_CMD;
    frame->data[2] = AG_CMD_ID;
    frame->flags |= AG_FRAME_FLAG_VALID;
    ag_comm_tx(frame);
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_mod_reset(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 1) {
        return CMD_WRONG_N;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->params[0], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CMD_DONE;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CMD_DONE;
    }

    AG_FRAME_L0 *frame = ag_comm_get_tx_frame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CMD_DONE;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_PROTO_VER1;
    frame->data[1] = AG_PKT_TYPE_CMD;
    frame->data[2] = AG_CMD_RESET;
    frame->flags |= AG_FRAME_FLAG_VALID;
    ag_comm_tx(frame);
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_mod_power_on(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 1) {
        return CMD_WRONG_N;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->params[0], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CMD_DONE;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CMD_DONE;
    }

    AG_FRAME_L0 *frame = ag_comm_get_tx_frame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CMD_DONE;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_PROTO_VER1;
    frame->data[1] = AG_PKT_TYPE_CMD;
    frame->data[2] = AG_CMD_POWER_ON;
    frame->flags |= AG_FRAME_FLAG_VALID;
    ag_comm_tx(frame);
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_mod_power_off(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 1) {
        return CMD_WRONG_N;
    }

    uint8_t mc_id = (uint8_t) strtol(cmdp->params[0], NULL, 10);
    if (mc_id >= AG_MC_MAX_CNT) {
        printf("INCORRECT id\n");
        return CMD_DONE;
    }
    if (REMOTE_MODS[mc_id].last_seen == -1) {
        printf("CANNOT send message to %d\n", mc_id);
        return CMD_DONE;
    }

    AG_FRAME_L0 *frame = ag_comm_get_tx_frame();
    if (frame == NULL) {
        printf("%s - CANNOT get TX frame\n", __func__);
        return CMD_DONE;
    }

    frame->dst_mac[1] = REMOTE_MODS[mc_id].mac[1];
    frame->dst_mac[0] = REMOTE_MODS[mc_id].mac[0];
    frame->data[0] = AG_PROTO_VER1;
    frame->data[1] = AG_PKT_TYPE_CMD;
    frame->data[2] = AG_CMD_POWER_OFF;
    frame->flags |= AG_FRAME_FLAG_VALID;
    ag_comm_tx(frame);
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_pwr_stats(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 0) {
        return CMD_WRONG_N;
    }

    printf("V5_SRC = OFF\n");
    printf("V5_LOAD = OFF\n");
    printf("I5 = %.3f A\n", ag_get_I5_NOM());
    printf("I3 = %.3f A\n", ag_get_I3_NOM());
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_pwr_ctrl(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 0) {
        return CMD_WRONG_N;
    }

    printf("SRC_I5_NOM = %.3f A\n", MOD_STATE.i5_nom);
    printf("SRC_I5_CUTOFF = %.3f A\n", MOD_STATE.i5_cutoff);
    printf("SRC_I3_NOM = %.3f A\n", MOD_STATE.i3_nom);
    printf("SRC_I3_CUTOFF = %.3f A\n", MOD_STATE.i3_cutoff);

    printf("LOAD_I5_NOM = %.3f A\n", MOD_STATE.i5_nom);
    printf("LOAD_I5_CUTOFF = %.3f A\n", MOD_STATE.i5_cutoff);
    printf("LOAD_I3_NOM = %.3f A\n", MOD_STATE.i3_nom);
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_pwr_v5_src(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 1) {
        return CMD_WRONG_N;
    }

    if (strncmp(cmdp->params[0], "on", 2) == 0) {
        printf("on\n");
    } else if (strncmp(cmdp->params[0], "off", 3) == 0) {
        printf("off\n");
    } else {
        return CMD_WRONG_N;
    }
    return CMD_DONE;
}

CLI_CMD_RETURN_t cmd_pwr_v5_load(CLI_PARSED_CMD_t *cmdp) {
    if (cmdp->nParams != 1) {
        return CMD_WRONG_N;
    }

    if (strncmp(cmdp->params[0], "on", 2) == 0) {
        printf("on\n");
    } else if (strncmp(cmdp->params[0], "off", 3) == 0) {
        printf("off\n");
    } else {
        return CMD_WRONG_N;
    }
    return CMD_DONE;
}
