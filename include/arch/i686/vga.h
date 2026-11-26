/*
 * Project Genesis - VGA text console (i686).
 *
 * The reference machine has no serial port, so the screen is the only way to
 * see anything on real hardware. Text mode 3 (80x25, 16 colours) lives at
 * physical 0xB8000 and needs no driver, no framebuffer and no paging - which is
 * exactly what a very light kernel wants.
 */

#ifndef GENESIS_ARCH_I686_VGA_H
#define GENESIS_ARCH_I686_VGA_H

#include <stdint.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Clear the screen and park the cursor at the top-left corner. */
void vga_init(void);

/* Write one character, handling '\n', '\r' and '\t', and scroll at the last
 * line. Also keeps the hardware cursor in sync. */
void vga_putc(char c);

#endif /* GENESIS_ARCH_I686_VGA_H */
