// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AGATHIS_COMM_ZC5DS878HG83B98T
#define AGATHIS_COMM_ZC5DS878HG83B98T
/** @file */

#include <stdint.h>

#define AG_FRM_DATA_LEN     16

#define AG_FRM_FLAG_APP_RD      0x01
#define AG_FRM_FLAG_APP_WR      0x02
#define AG_FRM_FLAG_PHY_RX      0x04
#define AG_FRM_FLAG_PHY_TX      0x08

#define AG_FRM_PROTO_VER1       1

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
    uint8_t flags;
    uint8_t nb;
    uint8_t *data;
} AG_FRAME_L0;

int agComm_IsFrameFromMaster(AG_FRAME_L0 *frame);

int agComm_IsFrameSts(AG_FRAME_L0 *frame);

int agComm_IsFrameCmd(AG_FRAME_L0 *frame);

int agComm_IsFrameAck(AG_FRAME_L0 *frame);

AG_FRAME_L0 *agComm_GetTXFrame(void);

AG_FRAME_L0 *agComm_GetRXFrame(void);

void agComm_SendFrame(void);

void agComm_RecvFrame(void);

void agComm_SetFrameType(AGFrmType_t type, AG_FRAME_L0 *frame);

void agComm_SetFrameCmd(AGFrmCmd_t cmd, AG_FRAME_L0 *frame);

AGFrmCmd_t agComm_GetFrameCmd(AG_FRAME_L0 *frame);

void agComm_SetFrameRTS(AG_FRAME_L0 *frame);

void agComm_SetFrameDone(AG_FRAME_L0 *frame);

void agComm_Init(void);

void agComm_SendStatus(void);

#endif /* AGATHIS_COMM_ZC5DS878HG83B98T */
