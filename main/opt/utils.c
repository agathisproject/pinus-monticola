// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

#include "../hw/platform.h"

void log_file(const char *tag, const char *format, ...) {
#if defined(__linux)
    FILE *fp;
    const char *f_path = platform_GetParamStr(PLTF_LOG_PATH);
    va_list args;
    struct timeval t_now;

    gettimeofday(&t_now, NULL);
    fp = fopen(f_path, "a");
    if (!fp) {
        return;
    }
    va_start(args, format);
    fprintf(fp, "%ld.%06ld,%s,", t_now.tv_sec, t_now.tv_usec, tag);
    vfprintf(fp, format, args);
    fprintf(fp, "\n");
    va_end(args);
    fclose(fp);
#endif
}

uint32_t diff_ms(struct timeval t1, struct timeval t2) {
    return (uint32_t) ((t1.tv_sec - t2.tv_sec) * 1000) + (uint32_t) ((t1.tv_usec - t2.tv_usec) / 1000);
}
