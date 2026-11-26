/*
 * Project Genesis - boot splash (i686).
 *
 * The first thing the kernel does is paint a picture over the whole text
 * screen, so a working boot is visible at a glance - the reference laptop has
 * no serial port, and a silent black screen is indistinguishable from a crash.
 * The splash then hands the screen back to the console for the boot report.
 *
 * The art itself is generated (tools/gen-splash.py) and lives in
 * kernel/arch/i686/splash_art.c.
 */

#ifndef GENESIS_ARCH_I686_SPLASH_H
#define GENESIS_ARCH_I686_SPLASH_H

#include <stdint.h>

#define SPLASH_ART_COLUMNS 80
#define SPLASH_ART_ROWS    24

/* SPLASH_ART_ROWS lines of SPLASH_ART_COLUMNS characters, NUL-terminated. */
extern const char splash_art[SPLASH_ART_ROWS][SPLASH_ART_COLUMNS + 1];

/* Paint the art over the full 80x25 screen and hide the cursor. */
void splash_show(void);

/* Wait roughly `milliseconds` without interrupts (8253/8254 timer channel 2).
 * Gives up and returns when no timer answers, so the boot can never hang. */
void splash_hold_ms(uint32_t milliseconds);

#endif /* GENESIS_ARCH_I686_SPLASH_H */
