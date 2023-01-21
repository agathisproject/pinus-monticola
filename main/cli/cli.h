// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLI_YP8GJVG3TCW7BWQW
#define CLI_YP8GJVG3TCW7BWQW
/** @file */

#include "cli_types.h"

typedef struct {
    CLICmdFolder_t *folder;
    char path[CLI_TREE_DEPTH_MAX][CLI_BASE_NAME_SIZE];
    uint8_t pathIdx;
} CLI_ENV_t;

/**
 * @brief set CLI prompt
 */
void cli_SetPrompt(const char *str);

/**
 * @brief init CLI
 */
void cli_Init(CLICmdFolder_t *menu);

/**
 * @brief get command from UART/stdin into the internal buffer
 */
void cli_Read(void);

/**
 * @brief parse command from internal buffer
 */
void cli_Parse(void);

/**
 * @brief execute command
 */
void cli_Execute(void);

#endif /* CLI_YP8GJVG3TCW7BWQW */
