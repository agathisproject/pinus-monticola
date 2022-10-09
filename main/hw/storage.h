// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STORAGE_WJQAWZ7F2E5SSJEN
#define STORAGE_WJQAWZ7F2E5SSJEN
/** @file */

#include <stdint.h>

void stor_get_MAC(uint8_t *mac);

void stor_get_MAC_compact(uint32_t *mac);

void stor_restore_state(void);

void stor_save_state(void);

#endif /* STORAGE_WJQAWZ7F2E5SSJEN */
