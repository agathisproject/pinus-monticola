// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli.h"

#include <stdio.h>
#include <string.h>

#include "cmd.h"

#if defined(__AVR__)
#include "../mcc_generated_files/uart1.h"

uint8_t CLI_IsRxReady(void) {
    return UART1_IsRxReady();
}

uint8_t CLI_GetChar(void) {
    return (uint8_t) UART1_Read();
}
#elif defined(ESP_PLATFORM)
uint8_t p_CLI_IsRxReady(void) {
    return 1;
}

uint8_t p_CLI_GetChar(void) {
    return (uint8_t) getchar();
}
#elif defined(__linux__) || defined(__MINGW64__)
uint8_t p_CLI_IsRxReady(void) {
    return 1;
}

uint8_t p_CLI_GetChar(void) {
    return (uint8_t) getchar();
}
#endif

static char p_CLI_BUFF[CLI_BUFF_SIZE + 1] = { 0 };
static char p_CLI_PROMPT[CLI_PROMPT_SIZE] = { 0 };
static uint8_t p_no_cmd = 0;

static CLI_PARSED_CMD_t p_PARSED_CMD = {"\0", 0, {"", "", "", ""}};

static CLI_ENV_t p_CLI_ENV;

char * CLI_getPrompt(void) {
    return p_CLI_PROMPT;
}

void CLI_setPrompt(const char *str) {
    strncpy(p_CLI_PROMPT, str, CLI_PROMPT_SIZE);
}

static CLI_CMD_t p_cmd_root[3]  = {
    {"info", "", "show module info", &cmd_info},
    {"set",  "", "change configuration", &cmd_set},
    {"save", "", "save configuration", &cmd_save},
};
static CLI_FOLDER_t p_f_root = {"", sizeof(p_cmd_root) / sizeof(p_cmd_root[0]), p_cmd_root,
                                NULL, NULL, NULL, NULL, NULL
                               };

static CLI_CMD_t p_cmd_mod[5]  = {
    {"info", "", "show modules", &cmd_mod_info},
    {"id", "", "identify module", &cmd_mod_id},
    {"reset", "", "reset module", &cmd_mod_reset},
    {"on", "", "power on module", &cmd_mod_power_on},
    {"off", "", "power off module", &cmd_mod_power_off},
};
static CLI_FOLDER_t p_f_mod = {"mod", sizeof(p_cmd_mod) / sizeof(p_cmd_mod[0]), p_cmd_mod,
                               &p_cmd_mod[0], NULL, NULL, NULL, NULL
                              };

static CLI_FOLDER_t p_f_lcl = {"lcl", 0, NULL, NULL, NULL, NULL, NULL, NULL};

#if MOD_HAS_PWR
static CLI_CMD_t p_cmd_pwr[4]  = {
    {"stats",  "", "show stats", &cmd_pwr_stats},
    {"ctrl",   "", "show controls", &cmd_pwr_ctrl},
    {"v5_src",  "[on|off]", "V5 source switch", &cmd_pwr_v5_src},
    {"v5_load", "[on|off]", "V5 load switch", &cmd_pwr_v5_load},
};
static CLI_FOLDER_t p_f_pwr  = {"pwr", sizeof(p_cmd_pwr) / sizeof(p_cmd_pwr[0]), p_cmd_pwr,
                                &p_cmd_pwr[0], NULL, NULL, NULL, NULL
                               };
#else
static CLI_FOLDER_t p_f_pwr  = {"pwr", 0, NULL, NULL, NULL, NULL, NULL, NULL};
#endif

static CLI_FOLDER_t p_f_clk  = {"clk", 0, NULL, NULL, NULL, NULL, NULL, NULL};
static CLI_FOLDER_t p_f_pps  = {"pps", 0, NULL, NULL, NULL, NULL, NULL, NULL};
static CLI_FOLDER_t p_f_jtag = {"jtag", 0, NULL, NULL, NULL, NULL, NULL, NULL};
static CLI_FOLDER_t p_f_usb  = {"usb", 0, NULL, NULL, NULL, NULL, NULL, NULL};
static CLI_FOLDER_t p_f_pcie = {"pcie", 0, NULL, NULL, NULL, NULL, NULL, NULL};

void CLI_init(void) {
    unsigned int i = 0;

    p_f_root.parent = &p_f_root;
    p_f_root.child = &p_f_lcl;

    p_f_lcl.parent = &p_f_root;
    p_f_lcl.right = &p_f_mod;
    p_f_lcl.child = &p_f_pwr;

    p_f_mod.parent = &p_f_root;
    p_f_mod.left = &p_f_lcl;

    p_f_pwr.parent = &p_f_lcl;
    p_f_pwr.right = &p_f_clk;

    p_f_clk.parent = &p_f_lcl;
    p_f_clk.left = &p_f_pwr;
    p_f_clk.right = &p_f_pps;

    p_f_pps.parent = &p_f_lcl;
    p_f_pps.left = &p_f_clk;
    p_f_pps.right = &p_f_jtag;

    p_f_jtag.parent = &p_f_lcl;
    p_f_jtag.left = &p_f_pps;
    p_f_jtag.right = &p_f_usb;

    p_f_usb.parent = &p_f_lcl;
    p_f_usb.left = &p_f_jtag;
    p_f_usb.right = &p_f_pcie;

    p_f_pcie.parent = &p_f_lcl;
    p_f_pcie.left = &p_f_usb;

    for (i = 0; i < CLI_TREE_DEPTH_MAX; i++) {
        strncpy(p_CLI_ENV.path[i], "\0", CLI_BASE_NAME_SIZE);
    }
    p_CLI_ENV.pathIdx = 0;
    p_CLI_ENV.folder = &p_f_root;
}

void CLI_getCmd(void) {
    uint8_t byteIn;
    uint8_t idx = 0;

    strncpy(p_CLI_BUFF, "\0", (CLI_BUFF_SIZE + 1));
    while (1) {
        if (p_CLI_IsRxReady()) {
            byteIn = p_CLI_GetChar();
            //printf("DBG: %x\n", byteIn);

            // NL or CR end the command
            if ((byteIn == 10) || (byteIn == 13)) {
                break;
            }
            // BS removes last character from buffer
            if (byteIn == 8) {
                if (idx > 0) {
                    idx --;
                    p_CLI_BUFF[idx] = '\0';
                }
                continue;
            }
            // skip non-printable characters
            if ((byteIn < 32) || (byteIn > 126)) {
                continue;
            }
            // if buffer full skip adding
            if (idx == CLI_BUFF_SIZE) {
                continue;
            }
            p_CLI_BUFF[idx] = (char)byteIn;
            idx ++;
            p_CLI_BUFF[idx] = '\0';
        }
    }
}

uint8_t CLI_parseCmd(void) {
    strncpy(p_PARSED_CMD.cmd, "\0", CLI_WORD_SIZE);
    p_PARSED_CMD.nParams = 0;
    for (uint8_t i = 0; i < CLI_WORD_CNT; i++) {
        strncpy(p_PARSED_CMD.params[i], "\0", CLI_WORD_SIZE);
    }

    //printf("DBG: parse ...\n");
    uint8_t j = 0;
    for (uint8_t i = 0; i < CLI_BUFF_SIZE; i ++) {
        if (p_CLI_BUFF[i] == 32) {
            p_PARSED_CMD.nParams ++;
            j = 0;
            continue;
        }
        if (p_CLI_BUFF[i] == 0) {
            p_PARSED_CMD.nParams ++;
            break;
        }

        if (j >= CLI_WORD_SIZE) {
            printf("ERROR parsing (size)\n");
            return 1;
        }
        if (p_PARSED_CMD.nParams >= CLI_WORD_CNT) {
            printf("ERROR parsing (param)\n");
            return 2;
        }

        if (p_PARSED_CMD.nParams == 0) {
            p_PARSED_CMD.cmd[j] = p_CLI_BUFF[i];
            j ++;
            p_PARSED_CMD.cmd[j] = '\0';
        } else {
            p_PARSED_CMD.params[p_PARSED_CMD.nParams - 1][j] = p_CLI_BUFF[i];
            j ++;
            p_PARSED_CMD.params[p_PARSED_CMD.nParams - 1][j] = '\0';
        }
    }
    if (p_PARSED_CMD.nParams > 0) {
        p_PARSED_CMD.nParams --;
    }
    return 0;
}

CLI_CMD_RETURN_t p_help(void) {
    printf("built-in commands:\n");
    printf("  pwd - show current path\n");
    printf("   ls - list possible commands\n");
    printf("   cd - change folder\n");

    return CMD_DONE;
}

CLI_CMD_RETURN_t p_pwd(void) {
    unsigned int i = 0;

    if (p_CLI_ENV.pathIdx == 0) {
        printf("/\n");
    } else {
        for (i = 0; i < p_CLI_ENV.pathIdx; i++) {
            printf("/%s", p_CLI_ENV.path[i]);
        }
        printf("\n");
    }
    return CMD_DONE;
}

CLI_CMD_RETURN_t p_ls(void) {
    unsigned int i = 0;

    for (i = 0; i < p_CLI_ENV.folder->nCmds; i ++) {
        printf("    %s - %s\n", p_CLI_ENV.folder->cmds[i].cmd,
               p_CLI_ENV.folder->cmds[i].cmdHelp);
    }

    if (p_CLI_ENV.folder->child != NULL) {
        CLI_FOLDER_t *tmp = p_CLI_ENV.folder->child;
        while (tmp != NULL) {
            printf("[+] %s\n", tmp->name);
            tmp = tmp->right;
        }
    }
    return CMD_DONE;
}

CLI_CMD_RETURN_t p_cd(void) {
    if (p_PARSED_CMD.nParams == 0) {
        p_CLI_ENV.folder = &p_f_root;
        p_CLI_ENV.pathIdx = 0;
        return CMD_DONE;
    }

    if (p_PARSED_CMD.nParams > 1) {
        return CMD_WRONG_N;
    }

    if (strncmp(p_PARSED_CMD.params[0], "..", 2) == 0) {
        p_CLI_ENV.folder = p_CLI_ENV.folder->parent;
        p_CLI_ENV.pathIdx--;
        return CMD_DONE;
    }

    if (p_CLI_ENV.folder->child == NULL) {
        printf("UNKNOWN path: %s\n", p_PARSED_CMD.params[0]);
        return CMD_WRONG_PARAM;
    }

    CLI_FOLDER_t *tmp = p_CLI_ENV.folder->child;
    while (tmp != NULL) {
        if (strncmp(p_PARSED_CMD.params[0], tmp->name, CLI_BASE_NAME_SIZE) == 0) {
            p_CLI_ENV.folder = tmp;
            strncpy(p_CLI_ENV.path[p_CLI_ENV.pathIdx++], tmp->name, CLI_BASE_NAME_SIZE);
            return CMD_DONE;
        }
        tmp = tmp->right;
    }
    printf("UNKNOWN path: %s\n", p_PARSED_CMD.params[0]);
    return CMD_DONE;
}

void CLI_execute(void) {
    unsigned int i = 0;
    CLI_CMD_RETURN_t cmdRet = CMD_NOT_FOUND;

    //printf("DBG: execute %s (%d params) %d\n", p_PARSED_CMD.cmd, p_PARSED_CMD.nParams, p_CLI_ENV.folder);
    printf("\n");
    if (strlen(p_PARSED_CMD.cmd) == 0) {
        if (p_CLI_ENV.folder->cmdDefault == NULL) {
            if (p_no_cmd == 4) {
                printf("press ? for help\n");
                p_no_cmd = 0;
            } else {
                p_no_cmd ++;
            }
            cmdRet = CMD_DONE;
        } else {
            cmdRet = p_CLI_ENV.folder->cmdDefault->fptr(&p_PARSED_CMD);
        }
    } else if (strncmp(p_PARSED_CMD.cmd, "?", 1) == 0) {
        cmdRet = p_help();
    } else if (strncmp(p_PARSED_CMD.cmd, "pwd", 3) == 0) {
        cmdRet = p_pwd();
    } else if (strncmp(p_PARSED_CMD.cmd, "ls", 2) == 0) {
        cmdRet = p_ls();
    } else if (strncmp(p_PARSED_CMD.cmd, "cd", 2) == 0) {
        cmdRet = p_cd();
    } else {
        for (i = 0; i < p_CLI_ENV.folder->nCmds; i++) {
            if (strncmp(p_PARSED_CMD.cmd, p_CLI_ENV.folder->cmds[i].cmd,
                        CLI_WORD_SIZE) == 0) {
                cmdRet = p_CLI_ENV.folder->cmds[i].fptr(&p_PARSED_CMD);
                break;
            }
        }
    }

    if (cmdRet == CMD_NOT_FOUND) {
        printf("UNRECOGNIZED command\n");
    } else if (cmdRet == CMD_WRONG_N) {
        printf("WRONG argument count\n");
        //printf("  %s %s\n", p_CLI_ENV.folder->cmds[i].cmd, p_CLI_ENV.folder->cmds[i].argDesc);
    } else if (cmdRet == CMD_WRONG_PARAM) {
        printf("WRONG arguments\n");
        //printf("  %s %s\n", p_CLI_ENV.folder->cmds[i].cmd, p_CLI_ENV.folder->cmds[i].argDesc);
    }
}
