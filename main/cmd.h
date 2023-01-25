// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CMD_H868VY2FS9UU4CP6
#define CMD_H868VY2FS9UU4CP6
/** @file */

#include <stdint.h>

#include "cli/cli_types.h"
#include "agathis/config.h"

#define APP_NAME "pinus-monitcola"

void cmd_SetPrompt(void);

CLIStatus_t cmd_Info(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_Set(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_Cfg(CLICmdParsed_t *cmdp);

CLIStatus_t cmd_ModInfo(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_ModLed(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_ModReset(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_ModPowerOn(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_ModPowerOff(CLICmdParsed_t *cmdp);
CLIStatus_t cmd_ModSpeed(CLICmdParsed_t *cmdp);

#endif /* CMD_H868VY2FS9UU4CP6 */
