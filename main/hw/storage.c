// SPDX-License-Identifier: GPL-3.0-or-later

#include "storage.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__linux__)
#include "../sim/state.h"
#elif defined (CONFIG_IDF_TARGET)
#include "esp_mac.h"
#endif

#include "../agathis/base.h"

/**
 * @brief returns the MAC address
 * @param mac array of 6 * uint8_t
 */
void stor_get_MAC(uint8_t *mac) {
#if defined(__linux__)
    for (int i = 0; i < 6; i++) {
        mac[i] = SIM_STATE.mac[i];
    }
#elif defined (CONFIG_IDF_TARGET)
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buff));
    for (int i = 0; i < 6; i++) {
        mac[0] = buff[5 - i];
    }
#endif
}

/**
 * @brief returns the MAC address
 * @param mac array of 2 * uint32_t
 */
void stor_get_MAC_compact(uint32_t *mac) {
#if defined(__linux__)
    // *INDENT-OFF*
    mac[1] = ((uint32_t) SIM_STATE.mac[5] << 16) | ((uint32_t) SIM_STATE.mac[4] << 8) | SIM_STATE.mac[3];
    mac[0] = ((uint32_t) SIM_STATE.mac[2] << 16) | ((uint32_t) SIM_STATE.mac[1] << 8) | SIM_STATE.mac[0];
    // *INDENT-ON*
#elif defined (CONFIG_IDF_TARGET)
    uint8_t buff[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buff));
    mac[1] = ((uint32_t) buff[0] << 16) | ((uint32_t) buff[1] << 8) | buff[2];
    mac[0] = ((uint32_t) buff[3] << 16) | ((uint32_t) buff[4] << 8) | buff[5];
#endif
}

static uint8_t p_get_eeprom_byte(uint16_t addr) {
#if defined(__AVR__)
    return 0xFF;
#elif defined(__XC16__)
    return 0xFF;
#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "rb");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, addr, SEEK_SET);
    uint8_t tmp = (uint8_t) fgetc(fp);
    fclose(fp);

    return tmp;
#elif defined (CONFIG_IDF_TARGET)
    return 0xFF; // TODO:
#endif
}

static void p_set_eeprom_byte(uint16_t addr, uint8_t data) {
#if defined(__AVR__)

#elif defined(__XC16__)

#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "r+b");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, addr, SEEK_SET);
    fputc(data, fp);
    fclose(fp);
#elif defined (CONFIG_IDF_TARGET)
    // TODO:
#endif
}

static void p_get_eeprom_str(uint16_t addr, uint8_t len, char *str) {
#if defined(__AVR__)
    memset(str, 0, len * sizeof(char) );
#elif defined(__XC16__)
    memset(str, 0, len * sizeof(char) );
#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "rb");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, addr, SEEK_SET);
    for (unsigned int i = 0; i < len; i++) {
        str[i] = (char) fgetc(fp);
    }
    str[len] = '\0';

    fclose(fp);
#elif defined (CONFIG_IDF_TARGET)
    memset(str, 0x00, len); // TODO:
#endif
}

/**
 * read floating point value from EEPROM
 *
 * Reads a 16b value from EEPROM as a integer, then divide by 100
 * Possible values are between -327.68 and 327.67
 * @param addr
 * @param f
 * @return
 */
static float p_get_eeprom_float(uint16_t addr) {
#if defined(__AVR__)
    return 0.0;
#elif defined(__XC16__)
    return 0.0;
#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "rb");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    int16_t tmp = 0;
    fseek(fp, addr, SEEK_SET);
    tmp = (int16_t) fgetc(fp);
    fseek(fp, (addr + 1), SEEK_SET);
    tmp += (int16_t) (fgetc(fp) << 8);

    fclose(fp);
    return ((float) tmp / 100.0f);
#elif defined (CONFIG_IDF_TARGET)
    return 0.0; // TODO:
#endif
}

void stor_restore_state(void) {
//    uint8_t ver = p_get_eeprom_byte(0);
//    printf("%s EEPROM ver %d\n", PREFIX_MC, ver);
//    if (ver == 0xFF) {
//        return;
//    }
//
//    uint8_t tmp = p_get_eeprom_byte(8);
//    if (tmp == 0xFF) {
//        MOD_STATE.caps_sw = 0x00;
//    } else {
//        MOD_STATE.caps_sw = tmp;
//    }
//
//    p_get_eeprom_str(16, 16, MOD_STATE.mfr_name);
//    p_get_eeprom_str(32, 16, MOD_STATE.mfr_pn);
//    p_get_eeprom_str(48, 16, MOD_STATE.mfr_sn);
//
//    MOD_STATE.i5_nom = p_get_eeprom_float(64);
//    MOD_STATE.i5_cutoff = p_get_eeprom_float(66);
//    MOD_STATE.i3_nom = p_get_eeprom_float(68);
//    MOD_STATE.i3_cutoff = p_get_eeprom_float(70);
}

void stor_save_state(void) {
//    uint8_t ver = p_get_eeprom_byte(0);
//    printf("%s EEPROM ver %d\n", PREFIX_MC, ver);
//    if (ver == 0xFF) {
//        p_set_eeprom_byte(0, 0x1);
//    }
//    p_set_eeprom_byte(8, MOD_STATE.caps_sw);
}
