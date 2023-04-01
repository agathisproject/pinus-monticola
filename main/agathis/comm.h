// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AGATHIS_COMM_ZC5DS878HG83B98T
#define AGATHIS_COMM_ZC5DS878HG83B98T
/** @file */

#include <stdint.h>

#define AG_FRM_DATA_LEN     16

#define AG_FRM_PROTO_VER1       1
#define AG_FRM_VER_OFFSET       0
#define AG_FRM_TYPE_OFFSET      1
#define AG_FRM_CMD_OFFSET       2

typedef enum {
    AG_RX_NONE = 0,
    AG_RX_FULL,
} AGRXState_t;

typedef enum {
    AG_FRM_TYPE_STS = 0,
    AG_FRM_TYPE_CMD,
    AG_FRM_TYPE_ACK,
} AGFrmType_t;

typedef enum {
    AG_FRM_CMD_ID = 1,
    AG_FRM_CMD_RESET,
    AG_FRM_CMD_POWER_ON,
    AG_FRM_CMD_POWER_OFF,
    AG_FRM_CMD_PING,
} AGFrmCmd_t;

typedef struct {
    uint32_t dst_mac[2];
    uint32_t src_mac[2];
    uint8_t data[AG_FRM_DATA_LEN];
} AG_FRAME_L0;

void agComm_InitTXFrame(AG_FRAME_L0 *frame);

void agComm_InitRXFrame(AG_FRAME_L0 *frame);

AGRXState_t agComm_GetRXState(void);

AGFrmType_t agComm_GetRXFrameType(void);

AGFrmCmd_t agComm_GetRXFrameCmd(void);

void agComm_CpRXFrame(AG_FRAME_L0 *frame);

int agComm_IsRXFrameFromBcast(void);

void agComm_SendFrame(AG_FRAME_L0 *frame);

void agComm_SetFrameType(AGFrmType_t type, AG_FRAME_L0 *frame);

void agComm_SetFrameCmd(AGFrmCmd_t cmd, AG_FRAME_L0 *frame);

void agComm_Init(void);

void agComm_Exit(void);

#endif /* AGATHIS_COMM_ZC5DS878HG83B98T */
