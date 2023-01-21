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
#define NVS_NAMESPACE "pinus"
#define NVS_KEY "MOD_STATE"
#endif

void stor_RestoreState(void) {
#if defined(ESP_PLATFORM)
    nvs_handle_t hndl_nvs;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &hndl_nvs);
    if (err != ESP_OK) {
        printf("E (%s) CANNOT nvs_open\n", __func__);
        return;
    }

    size_t bin_size = 0;
    // check if something is saved
    err = nvs_get_blob(hndl_nvs, NVS_KEY, NULL, &bin_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        printf("E (%s) CANNOT get blob\n", __func__);
        return;
    }

    if (bin_size == 0) {
        printf("W (%s) NO state saved\n", __func__ );
        return;
    }
    if (bin_size > AG_STORAGE_SIZE) {
        printf("E (%s) saved state size TOO BIG\n", __func__);
        return;
    }

    uint8_t *buff = (uint8_t *) malloc(bin_size * sizeof (uint8_t));
    if (buff == NULL) {
        printf("E (%s) CANNOT alloc buffer\n", __func__);
        return;
    }

    err = nvs_get_blob(hndl_nvs, NVS_KEY, buff, &bin_size);
    if (err != ESP_OK) {
        printf("E (%s) CANNOT read state\n", __func__);
        free(buff);
        return;
    }
    if (MOD_STATE.ver != buff[0]) {
        printf("W (%s) OLD state detected\n", __func__);
        free(buff);
        return;
    }
    nvs_close(hndl_nvs);
#else
    uint8_t *buff = (uint8_t *) malloc(AG_STORAGE_SIZE * sizeof (uint8_t));

    if (buff == NULL) {
        printf("E (%s) CANNOT alloc buffer\n", __func__);
        return;
    }
    memset(buff, 0, AG_STORAGE_SIZE);

    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "rb");
    if (!fp) {
        perror("CANNOT open file");
        free(buff);
        exit(EXIT_FAILURE);
    }

    size_t n_rd = fread(buff, sizeof (uint8_t), AG_STORAGE_SIZE, fp);
    fclose(fp);
    if (n_rd != AG_STORAGE_SIZE) {
        printf("E (%s) INCORRECT file read\n", __func__);
        free(buff);
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
    free(buff);

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
    nvs_handle_t hndl_nvs;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &hndl_nvs);
    if (err != ESP_OK) {
        printf("E (%s) CANNOT nvs_open\n", __func__);
        return;
    }

    uint8_t *buff = (uint8_t *) malloc(AG_STORAGE_SIZE * sizeof (uint8_t));
    if (buff == NULL) {
        printf("E (%s) CANNOT alloc buffer\n", __func__);
        return;
    }

    memset(buff, 0xFF, AG_STORAGE_SIZE);
    memcpy(buff, ((uint8_t *) &MOD_STATE), state_size);
    err = nvs_set_blob(hndl_nvs, NVS_KEY, buff, state_size);
    if (err != ESP_OK) {
        printf("E (%s) CANNOT save state\n", __func__);
        return;
        free(buff);
    }
    free(buff);
    nvs_close(hndl_nvs);
#elif (__linux__)
    uint8_t *buff = (uint8_t *) malloc(AG_STORAGE_SIZE * sizeof (uint8_t));

    if (buff == NULL) {
        printf("E (%s) CANNOT alloc buffer\n", __func__);
        return;
    }
    memset(buff, 0, AG_STORAGE_SIZE);
    memcpy(buff, ((uint8_t *) &MOD_STATE), state_size);

    const char *fName = SIM_STATE.eeprom_path;
    FILE *fp;

    fp = fopen(fName, "wb");
    if (!fp) {
        perror("CANNOT open file");
        free(buff);
        exit(EXIT_FAILURE);
    }

    size_t n_wr = fwrite(buff, sizeof (uint8_t), AG_STORAGE_SIZE, fp);
    fflush(fp);
    fclose(fp);
    if (n_wr != AG_STORAGE_SIZE) {
        printf("E (%s) INCORRECT file write\n", __func__);
        free(buff);
        return;
    }

    free(buff);
#endif
    printf("I (%d) state saved\n", __LINE__);
}

void stor_EraseState(void) {

}
