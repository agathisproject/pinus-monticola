// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTILS_QPM9PUKCDQEAYCT9
#define UTILS_QPM9PUKCDQEAYCT9
/** @file */

#include <stdint.h>
#include <sys/time.h>

void log_file(const char *tag, const char *format, ...);

uint32_t diff_ms(struct timeval t1, struct timeval t2);

#endif /* UTILS_QPM9PUKCDQEAYCT9 */
