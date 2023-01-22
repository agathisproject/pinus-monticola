// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli.h"

#include <stdio.h>
#include <string.h>

#if defined(ARDUINO)
#include "Arduino.h"
#elif defined(__AVR__)
#include "../mcc_generated_files/uart1.h"
#endif

#define FLAG_READ   0x01
#define FLAG_PARSED 0x02

uint8_t p_IsRXReady(void) {
#if defined(ARDUINO)
    return Serial.available();
#elif defined(__AVR__)
    return UART1_IsRxReady();
#elif defined(ESP_PLATFORM)
    return 1;
#elif defined(__linux__) || defined(__MINGW64__)
    return 1;
#endif
}

uint8_t p_GetRxChar(void) {
#if defined(ARDUINO)
    return Serial.read();
#elif defined(__AVR__)
    return (uint8_t) UART1_Read();
#elif defined(ESP_PLATFORM)
    return (uint8_t) getchar();
#elif defined(__linux__) || defined(__MINGW64__)
    return (uint8_t) getchar();
#endif
}

static char p_cli_prompt[CLI_PROMPT_MAX_LEN] = "";
static char p_cmd_buff[CLI_BUFF_SIZE] = "";
static uint8_t p_cmd_len = 0;
static uint8_t p_cmd_flags = 0;
static CLICmdParsed_t p_cmd_parsed;
static CLICmdFolder_t * p_cli_root;
static CLI_ENV_t p_CLI_ENV;
static uint8_t p_no_cmd_cnt = 0;

void cli_SetPrompt(const char *str) {
    strncpy(p_cli_prompt, str, CLI_PROMPT_MAX_LEN);
}

static void p_cmd_init(void) {
    memset(p_cmd_buff, 0x00, CLI_BUFF_SIZE);
    p_cmd_len = 0;
    p_cmd_flags = 0;

    p_cmd_parsed.nTk = 0;
    for (uint8_t i = 0; i < CLI_MAX_TOKENS; i ++) {
        memset(p_cmd_parsed.tokens[i], 0x00, CLI_TOKEN_MAX_LEN);
    }
}

static void p_cli_clear(void) {
    p_cmd_init();

#if defined(ARDUINO)
    Serial.print("~# ");
#else
    printf("%s", p_cli_prompt);
#endif
}

static void p_cli_error(CLIStatus_t err) {
#if defined(ARDUINO)
    Serial.println();
    Serial.print("CLI error: ");
    Serial.print(err, DEC);
    Serial.println();
#else
    printf("\nCLI error: %d\n", err);
#endif
    p_cli_clear();
}

void cli_Init(CLICmdFolder_t *menu) {
    p_cli_root = menu;

    for (unsigned int i = 0; i < CLI_TREE_DEPTH_MAX; i++) {
        memset(p_CLI_ENV.path[i], 0x00, CLI_BASE_NAME_SIZE);
    }
    p_CLI_ENV.pathIdx = 0;
    p_CLI_ENV.folder = p_cli_root;

    p_cli_clear();
}

void cli_Read(void) {
    char ch;

    while (p_IsRXReady()) {
        ch = (char) p_GetRxChar();

        if ((ch == 8) || (ch == 127)) { // backspace - is sometimes 127 in TTY
            if (p_cmd_len > 0) {
#if defined(ARDUINO)
                Serial.write(8);
#elif defined(ESP_PLATFORM)
                printf("%c %c", 8, 8);
#endif
                p_cmd_len --;
                p_cmd_buff[p_cmd_len] = 0;
            }
        }

        if ((ch == 10) || (ch == 13)) { // line-feed or carriage-return
#if defined(ARDUINO)
            Serial.println();
#elif defined(ESP_PLATFORM)
            printf("\n");
#endif
            p_cmd_flags |= FLAG_READ;
            break;
        }

        if ((ch > 31) && (ch < 127)) { // printable ASCII
#if defined(ARDUINO)
            Serial.write(ch);
#elif defined(ESP_PLATFORM)
            printf("%c", ch);
#endif
            if (p_cmd_len < CLI_BUFF_SIZE) {
                p_cmd_buff[p_cmd_len] = ch;
                p_cmd_len ++;
                if (p_cmd_len >= CLI_BUFF_SIZE) {
                    p_cli_error(CLI_CMD_TOO_LONG);
                }
                p_cmd_buff[p_cmd_len] = 0;
            }
        }
    }
}

void cli_Parse(void) {
    if ((p_cmd_flags & FLAG_READ) == 0) {
        return;
    }

    uint8_t tkLen = 0;
    for (uint8_t i = 0; i < p_cmd_len; i++) {
        if (p_cmd_buff[i] == 32) { //space
            if (tkLen != 0) {
                p_cmd_parsed.nTk ++;
                tkLen = 0;
            }
            if (p_cmd_parsed.nTk > CLI_MAX_TOKENS) {
                p_cli_error(CLI_TOO_MANY_TOKENS);
            }
        } else {
            p_cmd_parsed.tokens[p_cmd_parsed.nTk][tkLen] = p_cmd_buff[i];
            tkLen ++;
            if (tkLen >= CLI_TOKEN_MAX_LEN) {
                p_cli_error(CLI_TOKEN_TOO_LONG);
            }
        }
    }
    if (tkLen > 0) {
        p_cmd_parsed.nTk ++;
    }
    //Serial.print("DBG: parse ");
    //Serial.println(p_cmd_parsed.nTk, DEC);
    //printf("DBG: parse %d\n", p_cmd_parsed.nTk);
    p_cmd_flags |= FLAG_PARSED;
}

CLIStatus_t p_no_cmd(void) {
    if (p_CLI_ENV.folder->cmdDefault != NULL) {
        return p_CLI_ENV.folder->cmdDefault->fptr(&p_cmd_parsed);
    }
    if (p_no_cmd_cnt == 4) {
#if defined(ARDUINO)
        Serial.println("press ? for help");
#else
        printf("press ? for help\n");
#endif
        p_no_cmd_cnt = 0;
    } else {
        p_no_cmd_cnt ++;
    }
    return CLI_NO_ERROR;
}

CLIStatus_t p_help(void) {
    printf("built-in commands:\n");
    printf("  pwd - show current path\n");
    printf("   ls - list possible commands\n");
    printf("   cd - change folder\n");
    return CLI_NO_ERROR;
}

CLIStatus_t p_pwd(void) {
    unsigned int i = 0;

    if (p_CLI_ENV.pathIdx == 0) {
        printf("/\n");
    } else {
        for (i = 0; i < p_CLI_ENV.pathIdx; i++) {
            printf("/%s", p_CLI_ENV.path[i]);
        }
        printf("\n");
    }
    return CLI_NO_ERROR;
}

CLIStatus_t p_ls(void) {
    for (unsigned int i = 0; i < p_CLI_ENV.folder->nCmds; i ++) {
        printf("    %s %s - %s\n", p_CLI_ENV.folder->cmds[i].cmd,
               p_CLI_ENV.folder->cmds[i].argDesc,
               p_CLI_ENV.folder->cmds[i].cmdHelp);
    }

    if (p_CLI_ENV.folder->child != NULL) {
        CLICmdFolder_t *tmp = p_CLI_ENV.folder->child;
        while (tmp != NULL) {
            printf("[+] %s\n", tmp->name);
            tmp = tmp->right;
        }
    }
    return CLI_NO_ERROR;
}

CLIStatus_t p_cd(void) {
    if (p_cmd_parsed.nTk == 1) {
        p_CLI_ENV.folder = p_cli_root;
        p_CLI_ENV.pathIdx = 0;
        return CLI_NO_ERROR;
    }

    if (p_cmd_parsed.nTk > 3) {
        return CLI_TOO_MANY_TOKENS;
    }

    if (strncmp(p_cmd_parsed.tokens[1], "..", 2) == 0) {
        p_CLI_ENV.folder = p_CLI_ENV.folder->parent;
        p_CLI_ENV.pathIdx--;
        return CLI_NO_ERROR;
    }

    if (p_CLI_ENV.folder->child == NULL) {
        printf("UNKNOWN path: %s\n", p_cmd_parsed.tokens[1]);
        return CLI_TOKEN_TOO_LONG;
    }

    CLICmdFolder_t *tmp = p_CLI_ENV.folder->child;
    while (tmp != NULL) {
        if (strncmp(p_cmd_parsed.tokens[1], tmp->name, CLI_BASE_NAME_SIZE) == 0) {
            p_CLI_ENV.folder = tmp;
            strncpy(p_CLI_ENV.path[p_CLI_ENV.pathIdx++], tmp->name, CLI_BASE_NAME_SIZE);
            return CLI_NO_ERROR;
        }
        tmp = tmp->right;
    }
    printf("UNKNOWN path: %s\n", p_cmd_parsed.tokens[1]);
    return CLI_NO_ERROR;
}

void cli_Execute(void) {
    if ((p_cmd_flags & FLAG_PARSED) == 0) {
        return;
    }
    //printf("DBG: execute %s (%d tokens) %s\n", p_cmd_parsed.tokens[0], p_cmd_parsed.nTk, p_CLI_ENV.folder->name);

    CLIStatus_t sts = CLI_CMD_NOT_FOUND;
    if (p_cmd_parsed.nTk == 0) {
        sts = p_no_cmd();
    } else if (strncmp(p_cmd_parsed.tokens[0], "?", 1) == 0) {
        sts = p_help();
    } else if (strncmp(p_cmd_parsed.tokens[0], "help", 4) == 0) {
        sts = p_help();
    } else if (strncmp(p_cmd_parsed.tokens[0], "pwd", 3) == 0) {
        sts = p_pwd();
    } else if (strncmp(p_cmd_parsed.tokens[0], "ls", 2) == 0) {
        sts = p_ls();
    } else if (strncmp(p_cmd_parsed.tokens[0], "cd", 2) == 0) {
        sts = p_cd();
    } else {
        for (unsigned int i = 0; i < p_CLI_ENV.folder->nCmds; i++) {
            if (strncmp(p_cmd_parsed.tokens[0], p_CLI_ENV.folder->cmds[i].cmd,
                        CLI_TOKEN_MAX_LEN) == 0) {
                sts = p_CLI_ENV.folder->cmds[i].fptr(&p_cmd_parsed);
                break;
            }
        }
    }

    if (sts != CLI_NO_ERROR) {
        p_cli_error(sts);
    } else {
        p_cli_clear();
    }
}
