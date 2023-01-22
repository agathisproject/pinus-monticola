// SPDX-License-Identifier: GPL-3.0-or-later

#include "storage.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(ESP_PLATFORM)
#include "nvs_flash.h"
#elif defined(__linux__)
#include "platform_sim/state.h"
#endif

#include "../agathis/base.h"

#if defined(ESP_PLATFORM)
#define APP_NVS_NAMESPACE "pinus"
#define APP_NVS_KEY "MOD_STATE"
#endif

void stor_RestoreState(void) {
#if defined(ESP_PLATFORM)
    uint8_t buff[AG_STORAGE_SIZE];
    nvs_handle_t hndl_nvs;

    memset(buff, 0xFF, AG_STORAGE_SIZE);
    ESP_ERROR_CHECK( nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, &hndl_nvs) );

    size_t bin_size = 0;
    // check if something is saved
    ESP_ERROR_CHECK_WITHOUT_ABORT( nvs_get_blob(hndl_nvs, APP_NVS_KEY, NULL,
                                   &bin_size) );
    if (bin_size == 0) {
        printf("W (%s) NO state saved\n", __func__ );
        return;
    }
    if (bin_size > AG_STORAGE_SIZE) {
        printf("E (%s) saved state size TOO BIG\n", __func__);
        return;
    }

    ESP_ERROR_CHECK( nvs_get_blob(hndl_nvs, APP_NVS_KEY, buff, &bin_size) );
    nvs_close(hndl_nvs);
#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "rb");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    uint8_t buff[AG_STORAGE_SIZE];

    memset(buff, 0xFF, AG_STORAGE_SIZE);
    size_t n_rd = fread(buff, sizeof (uint8_t), AG_STORAGE_SIZE, fp);
    fclose(fp);
    if (n_rd != AG_STORAGE_SIZE) {
        printf("E (%s) INCORRECT file read\n", __func__);
        return;
    }
#endif
    if ((buff[0] == 0x00) || (buff[0] == 0xFF)) {
        printf("W (%s) NO state - using defaults\n", __func__);
    } else if (buff[0] == MOD_STATE.ver) {
        memcpy(((uint8_t *) &MOD_STATE), buff, (sizeof (MOD_STATE) / sizeof (uint8_t)));
        printf("I (%d) state restored\n", __LINE__);
    } else {
        printf("W (%s) DIFFERENT state - using defaults\n", __func__);
    }

    MOD_STATE.mfr_name[15] = 0x00;
    MOD_STATE.mfr_pn[15] = 0x00;
    MOD_STATE.mfr_sn[15] = 0x00;
}

void stor_SaveState(void) {
    size_t state_size = sizeof (MOD_STATE) / sizeof (uint8_t);
    if (state_size > AG_STORAGE_SIZE) {
        printf("E (%s) state size TOO BIG\n", __func__);
        return;
    }

#if defined(ESP_PLATFORM)
    uint8_t buff[AG_STORAGE_SIZE];
    nvs_handle_t hndl_nvs;

    memset(buff, 0xFF, AG_STORAGE_SIZE);
    ESP_ERROR_CHECK( nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, &hndl_nvs) );
    memcpy(buff, ((uint8_t *) &MOD_STATE), state_size);
    ESP_ERROR_CHECK( nvs_set_blob(hndl_nvs, APP_NVS_KEY, buff, state_size) );
    ESP_ERROR_CHECK( nvs_commit(hndl_nvs) );
    nvs_close(hndl_nvs);
#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "wb");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    uint8_t buff[AG_STORAGE_SIZE];

    memset(buff, 0xFF, AG_STORAGE_SIZE);
    memcpy(buff, ((uint8_t *) &MOD_STATE), state_size);
    size_t n_wr = fwrite(buff, sizeof (uint8_t), AG_STORAGE_SIZE, fp);
    fflush(fp);
    fclose(fp);
    if (n_wr != AG_STORAGE_SIZE) {
        printf("E (%s) INCORRECT file write\n", __func__);
        return;
    }
#endif
    printf("I (%d) state saved\n", __LINE__);
}

void stor_EraseState(void) {
#if defined(ESP_PLATFORM)
    nvs_handle_t hndl_nvs;

    ESP_ERROR_CHECK( nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, &hndl_nvs) );
    ESP_ERROR_CHECK( nvs_erase_all(hndl_nvs) );
    ESP_ERROR_CHECK( nvs_commit(hndl_nvs) );
    nvs_close(hndl_nvs);
#elif defined(__linux__)
    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "wb");
    if (!fp) {
        perror("CANNOT open file");
        exit(EXIT_FAILURE);
    }

    uint8_t buff[AG_STORAGE_SIZE];

    memset(buff, 0xFF, AG_STORAGE_SIZE);
    size_t n_wr = fwrite(buff, sizeof (uint8_t), AG_STORAGE_SIZE, fp);
    fflush(fp);
    fclose(fp);
    if (n_wr != AG_STORAGE_SIZE) {
        printf("E (%s) INCORRECT file write\n", __func__);
        return;
    }
#endif
}
