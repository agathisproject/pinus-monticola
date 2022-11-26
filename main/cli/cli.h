// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLI_YP8GJVG3TCW7BWQW
#define CLI_YP8GJVG3TCW7BWQW
/** @file */

#include <stdint.h>

#include "cli_types.h"

typedef struct {
    CLI_FOLDER_t *folder;
    char path[CLI_TREE_DEPTH_MAX][CLI_BASE_NAME_SIZE];
    uint8_t pathIdx;
} CLI_ENV_t;

/**
 * @return get CLI prompt
 */
char * CLI_getPrompt(void);

/**
 * @brief set CLI prompt
 */
void CLI_setPrompt(const char *str);

/**
 * @brief init CLI library
 */
void CLI_init(void);

/**
 * @brief get command from UART/stdin into the internal buffer
 */
void CLI_getCmd(void);

/**
 * @brief parse command from internal buffer
 *
 * @return 0 if no errors
 */
uint8_t CLI_parseCmd(void);

/**
 * @brief execute command
 */
void CLI_execute(void);

#endif /* CLI_YP8GJVG3TCW7BWQW */
