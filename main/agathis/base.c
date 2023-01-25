// SPDX-License-Identifier: GPL-3.0-or-later

#include "base.h"

#include <stdio.h>

#if defined(ESP_PLATFORM)
#include "../hw/platform_esp/espnow.h"
#elif defined(__linux__)
#include "../hw/platform_sim/state.h"
#endif

#include "config.h"
#include "../hw/storage.h"
#include "../hw/platform.h"

AGLclConfig_t g_MCConfig = {.ver = 1, .capsHW = 0, .capsSW = 0,
                            .type = 0, .mfrName = "", .mfrPN = "", .mfrSN = "",
                            .crc = 0xdeadbeef,
                           };

AGLclState_t g_MCState = {.lastErr = AG_ERR_NONE};

AGLclStats_t g_MCStats = {0, 0};

AGRmtState_t g_RemoteMCs[AG_MC_MAX_CNT] = {
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
    {.mac = {0, 0}, .capsSW = 0, .lastErr = 0, .lastSeen = -1},
};

static uint8_t cnt_id_led = 0;

void ag_Init(void) {
    stor_RestoreState();
    g_MCState.lastErr = AG_ERR_NONE;
}

void ag_AddRemoteMCInfo(const uint32_t *mac, uint8_t caps) {
    int idx_free = -1;
    for (int i = 0 ; i < AG_MC_MAX_CNT; i ++) {
        if ((g_RemoteMCs[i].lastSeen == -1) && (idx_free == -1)) {
            idx_free = i;
        }
        if ((g_RemoteMCs[i].mac[0] == mac[0]) && (g_RemoteMCs[i].mac[1] == mac[1])) {
            g_RemoteMCs[i].capsSW = caps;
            g_RemoteMCs[i].lastSeen = 0;
            return;
        }
    }

    if (idx_free >= 0) {
        g_RemoteMCs[idx_free].mac[1] = mac[1];
        g_RemoteMCs[idx_free].mac[0] = mac[0];
        g_RemoteMCs[idx_free].capsSW = caps;
        g_RemoteMCs[idx_free].lastSeen = 0;
#if defined(ESP_PLATFORM)
        espnow_add_peer(g_RemoteMCs[idx_free].mac[1], g_RemoteMCs[idx_free].mac[0]);
#endif
    } else {
        printf("CANNOT add MC - too many\n");
    }
}

void ag_UpdRemoteMCs(void) {
    for (int i = 0 ; i < AG_MC_MAX_CNT; i ++) {
        if (g_RemoteMCs[i].lastSeen == -1) {
            continue;
        }
        if (g_RemoteMCs[i].lastSeen > AG_MC_MAX_AGE) {
#if defined(ESP_PLATFORM)
            espnow_del_peer(g_RemoteMCs[i].mac[1], g_RemoteMCs[i].mac[0]);
#endif
            g_RemoteMCs[i].mac[1] = 0;
            g_RemoteMCs[i].mac[0] = 0;
            g_RemoteMCs[i].capsSW = 0;
            g_RemoteMCs[i].lastErr = AG_ERR_NONE;
            g_RemoteMCs[i].lastSeen = -1;
        } else {
            g_RemoteMCs[i].lastSeen += 1;
        }
    }
}

void ag_UpdAlarm(void) {
    int nm = 0;
    for (int i = 0 ; i < AG_MC_MAX_CNT; i ++) {
        if ((g_RemoteMCs[i].capsSW & AG_CAP_SW_TMC) != 0) {
            nm += 1;
        }
    }
    if ((g_MCConfig.capsSW & AG_CAP_SW_TMC) != 0) {
        nm += 1;
    }
    if (nm > 1) {
        g_MCState.lastErr = AG_ERR_MULTI_MASTER;
        return;
    }
    g_MCState.lastErr = AG_ERR_NONE;
}

void ag_UpdHWState(void) {
    uint32_t led_code = 0;

    if (g_MCState.lastErr != 0) {
        led_code |= 0x00FF0000U;
    } else {
        led_code &= ~0x00FF0000U;
    }

    if (cnt_id_led > 0) {
        led_code |= 0x000000FFU;
        cnt_id_led --;
    } else {
        led_code &= ~0x000000FFU;
    }

    gpio_SetRGB(led_code);
}

void ag_LEDCtrl(AGLEDState_t led) {
    printf("ID LED\n");
    cnt_id_led = 5;
}
