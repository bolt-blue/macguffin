/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>

typedef char byte;
typedef uint8_t u8;
typedef uint32_t u32;

/*
 * Crash on bad input during development
 */
#ifndef NDEBUG
#define ASSERT(exp) \
    if (!(exp)) { \
        fprintf(stderr, "[ASSERTION] (%s:%d:%s)\n", __FILE__, __LINE__, __func__); \
        *(volatile int *)0 = 0; \
    }
#else
#define ASSERT(exp)
#endif

#define KB(v) ((v) * 1024)
#define MB(v) ((v) * 1024 * 1024)
#define GB(v) ((v) * 1024 * 1024 * 1024)

#define reset_colour() fputs("\033[0m", stdout);
#define reset() fputs("\033[2J", stdout)
#define clear_screen() fputs("\033[H\033[J", stdout)
#define hidecur() fputs("\033[?25l", stdout)
#define showcur() fputs("\033[?25h", stdout)
#define gotoxy(x,y) fprintf(stdout, "\033[%d;%dH", (y), (x))

#define endswap32(v)            \
      ((v & 0xff)       << 24)  \
    | ((v & 0xff00)     << 8)   \
    | ((v & 0xff0000)   >> 8)   \
    | ((v & 0xff000000) >> 24)

#endif /* UTIL_H */
