// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AGATHIS_COMM_ZC5DS878HG83B98T
#define AGATHIS_COMM_ZC5DS878HG83B98T
/** @file */

#include <stdint.h>

#define AG_FRAME_LEN            16
#define AG_FRAME_FLAG_VALID     0x01

typedef struct {
    uint32_t dst_mac[2];
    uint32_t src_mac[2];
    uint8_t flags;
    uint8_t nb;
    uint8_t *data;
} AG_FRAME_L0;

int ag_comm_is_frame_master(AG_FRAME_L0 *frame);

AG_FRAME_L0 *ag_comm_get_tx_frame(void);

void ag_comm_init(void);

void ag_comm_main(void);

int ag_comm_tx(AG_FRAME_L0 *frame);

#endif /* AGATHIS_COMM_ZC5DS878HG83B98T */
