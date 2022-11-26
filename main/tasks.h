// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TASKS_32ME2VQU7SS244R9
#define TASKS_32ME2VQU7SS244R9

#include <stdint.h>

#if defined(__linux__)
void *task_cli(void *vargp);
void *task_rf(void *vargp);
#else
void task_cli(void *pvParameters);
void task_rf(void *pvParameters);
#endif

#endif /* TASKS_32ME2VQU7SS244R9 */
