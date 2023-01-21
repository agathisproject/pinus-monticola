// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CLI_BEES5G972K9T2FRP
#define CLI_BEES5G972K9T2FRP
/** @file */

#include <stdint.h>

#define CLI_BUFF_SIZE           64  //!< max command size
#define CLI_MAX_TOKENS          8   //!< max number of words for a command
#define CLI_TOKEN_MAX_LEN       20  //!< max command word size
#if defined(ARDUINO)
#define CLI_PROMPT_MAX_LEN      8
#else
#define CLI_PROMPT_MAX_LEN      32
#endif

typedef enum {
    CLI_NO_ERROR = 0,
    CLI_CMD_TOO_LONG,
    CLI_TOKEN_TOO_LONG,
    CLI_TOO_MANY_TOKENS,
    CLI_CMD_NOT_FOUND,
    CLI_CMD_PARAMS_ERROR,
    CLI_CMD_FAIL,
} CLIStatus_t;

typedef struct {
    uint8_t nTk;
    char tokens[CLI_MAX_TOKENS][CLI_TOKEN_MAX_LEN];
} CLICmdParsed_t;

#define CMD_ARG_DESC_SIZE 32
#define CMD_HELP_SIZE 32

typedef struct {
    const char cmd[CLI_TOKEN_MAX_LEN];      //!< command
    const char argDesc[CMD_ARG_DESC_SIZE];  //!< command arguments description
    const char cmdHelp[CMD_HELP_SIZE];      //!< command help/description
    CLIStatus_t (*fptr)(CLICmdParsed_t *cmdp);
} CLICmdDef_t;

typedef struct folder CLICmdFolder_t;

#define CLI_BASE_NAME_SIZE 8
#define CLI_TREE_DEPTH_MAX 8

struct folder {
    char name[CLI_BASE_NAME_SIZE];
    uint8_t nCmds;
    CLICmdDef_t *cmds;
    CLICmdDef_t *cmdDefault;
    CLICmdFolder_t *parent;
    CLICmdFolder_t *child;
    CLICmdFolder_t *left;
    CLICmdFolder_t *right;
};

#endif /* CLI_BEES5G972K9T2FRP */
