// SPDX-License-Identifier: GPL-3.0-or-later

#include "base.h"

#include <stdio.h>

#if defined(__AVR__)
#include <avr/wdt.h>
#elif defined(ESP_PLATFORM)
#include "../hw/platform_esp/base.h"
#include "../hw/platform_esp/espnow.h"
#elif defined(__linux__)
#include "../sim/state.h"
#include "../sim/misc.h"
#include "../hw/platform_sim/base.h"
#endif

#include "config.h"
#include "../hw/storage.h"

AG_MC_STATE_t MOD_STATE = {.ver = 1, .caps_hw_ext = 0, .caps_hw_int = 0, .caps_sw = 0,
                           .last_err = 0, .type = 0, .tbd = 0xFF,
                           .mfr_name = "", .mfr_pn = "", .mfr_sn = "",
                           .i5_nom = 0.1f,  .i5_cutoff = 0.12f, .i3_nom = 1.0f, .i3_cutoff = 1.5f,
                           .crc = 0xdeadbeef,
                          };

AG_RMT_MC_STATE_t REMOTE_MODS[AG_MC_MAX_CNT] = {
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
    {.mac = {0, 0}, .caps = 0, .last_err = 0, .last_seen = -1},
};

static uint8_t cnt_id_led = 0;

void ag_init(void) {
#if MOD_HAS_STORAGE
    MOD_STATE.caps_hw_int = AG_CAP_INT_STORAGE;
#endif

#if MOD_HAS_PWR
    MOD_STATE.caps_hw_ext |= AG_CAP_EXT_PWR;
#endif
#if MOD_HAS_CLK
    MOD_STATE.caps_hw_ext |= AG_CAP_EXT_CLK;
#endif
#if MOD_HAS_1PPS
    MOD_STATE.caps_hw_ext |= AG_CAP_EXT_1PPS;
#endif
#if MOD_HAS_JTAG
    MOD_STATE.caps_hw_ext |= AG_CAP_EXT_JTAG;
#endif
#if MOD_HAS_USB
    MOD_STATE.caps_hw_ext |= AG_CAP_EXT_USB;
#endif
#if MOD_HAS_PCIE
    MOD_STATE.caps_hw_ext |= AG_CAP_EXT_PCIE;
#endif

#if MOD_HAS_STORAGE
    stor_restore_state();
#endif
    MOD_STATE.last_err = AG_ERR_NONE;
}

void ag_reset(void) {
#if defined(__AVR__)
    printf("reset\n");
    wdt_enable(WDTO_15MS);
#elif defined(__linux__)
    printf("RESET !!!\n");
#endif
}

void ag_add_remote_mod(const uint32_t *mac, uint8_t caps) {
    int idx_free = -1;
    for (int i = 0 ; i < AG_MC_MAX_CNT; i ++) {
        if ((REMOTE_MODS[i].last_seen == -1) && (idx_free == -1)) {
            idx_free = i;
        }
        if ((REMOTE_MODS[i].mac[0] == mac[0]) && (REMOTE_MODS[i].mac[1] == mac[1])) {
            REMOTE_MODS[i].caps = caps;
            REMOTE_MODS[i].last_seen = 0;
            return;
        }
    }

    if (idx_free >= 0) {
        REMOTE_MODS[idx_free].mac[1] = mac[1];
        REMOTE_MODS[idx_free].mac[0] = mac[0];
        REMOTE_MODS[idx_free].caps = caps;
        REMOTE_MODS[idx_free].last_seen = 0;
#if defined(ESP_PLATFORM)
        espnow_add_peer(REMOTE_MODS[idx_free].mac[1], REMOTE_MODS[idx_free].mac[0]);
#endif
    } else {
        printf("CANNOT add MC - too many\n");
    }
}

void ag_upd_remote_mods(void) {
    for (int i = 0 ; i < AG_MC_MAX_CNT; i ++) {
        if (REMOTE_MODS[i].last_seen == -1) {
            continue;
        }
        if (REMOTE_MODS[i].last_seen > AG_MC_MAX_AGE) {
#if defined(ESP_PLATFORM)
            espnow_del_peer(REMOTE_MODS[i].mac[1], REMOTE_MODS[i].mac[0]);
#endif
            REMOTE_MODS[i].mac[1] = 0;
            REMOTE_MODS[i].mac[0] = 0;
            REMOTE_MODS[i].caps = 0;
            REMOTE_MODS[i].last_err = AG_ERR_NONE;
            REMOTE_MODS[i].last_seen = -1;
        } else {
            REMOTE_MODS[i].last_seen += 1;
        }
    }
}

void ag_upd_alarm(void) {
    int nm = 0;
    for (int i = 0 ; i < AG_MC_MAX_CNT; i ++) {
        if ((REMOTE_MODS[i].caps & AG_CAP_SW_TMC) != 0) {
            nm += 1;
        }
    }
    if ((MOD_STATE.caps_sw & AG_CAP_SW_TMC) != 0) {
        nm += 1;
    }
    if (nm > 1) {
        MOD_STATE.last_err = AG_ERR_MULTI_MASTER;
        return;
    }
    MOD_STATE.last_err = AG_ERR_NONE;
}

void ag_upd_hw(void) {
    uint32_t led_code = 0;

    if (MOD_STATE.last_err != 0) {
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

    gpio_RGB_send(led_code);
}

void ag_id_external(void) {
    printf("ID LED\n");
    cnt_id_led = 5;
}

void ag_brd_pwr_off(void) {
    printf("board POWER OFF\n");
}

void ag_brd_pwr_on(void) {
    printf("board POWER ON\n");
}

float ag_get_I5_NOM(void) {
#if defined(__AVR__)
    return 0.0f;
#elif defined(ESP_PLATFORM)
    return 0.0f;
#elif defined(__linux__)
    return getValue_random((MOD_STATE.i5_nom * 0.7f), 5);
#endif
}

float ag_get_I3_NOM(void) {
#if defined(__AVR__)
    return 0.0f;
#elif defined(ESP_PLATFORM)
    return 0.0f;
#elif defined(__linux__)
    return getValue_random((MOD_STATE.i3_nom * 0.5f), 5);
#endif
}
