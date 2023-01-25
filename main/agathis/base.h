// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AGATHIS_6PLS6RVRFVYEP7NX
#define AGATHIS_6PLS6RVRFVYEP7NX
/** @file */

#include <stdint.h>

#include "defs.h"

/**
 * @brief configuration of the local MC (Management Controller). Values that SHOULD be saved/restored.
 */
typedef struct {
    uint8_t ver;          /**< version */
    uint8_t capsHW;       /**< HW capabilities */
    uint8_t capsSW;       /**< SW capabilities */
    uint16_t type;
    char mfrName[16];
    char mfrPN[16];
    char mfrSN[16];
    uint32_t crc;
} AGLclConfig_t;

extern AGLclConfig_t g_MCConfig;

/**
 * @brief state of the local MC (Management Controller). Values that SHOULD NOT be saved/restored.
 */
typedef struct {
    AGError_t lastErr;
} AGLclState_t;

extern AGLclState_t g_MCState;

typedef struct {
    uint32_t txCnt;
    uint32_t rxCnt;
} AGLclStats_t;

extern AGLclStats_t g_MCStats;

/**
 * @brief info about the other MCs (Management Controllers)
 */
typedef struct {
    uint32_t mac[2];
    uint8_t capsSW;
    uint8_t lastErr;
    int8_t lastSeen;
} AGRmtState_t;

#define AG_MC_MAX_CNT 16 /** max number of MCs in the chain, including the local one */
#define AG_MC_MAX_AGE 30

extern AGRmtState_t g_RemoteMCs[AG_MC_MAX_CNT];

void ag_Init(void);

void ag_AddRemoteMCInfo(const uint32_t *mac, uint8_t caps);

void ag_UpdRemoteMCs(void);

void ag_UpdAlarm(void);

void ag_UpdHWState(void);

void ag_LEDCtrl(AGLEDState_t led);

#endif /* AGATHIS_6PLS6RVRFVYEP7NX */
