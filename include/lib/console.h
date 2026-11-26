/*
 * Project Genesis - kernel console.
 *
 * One logical console that fans out to every output device available: the VGA
 * text screen always, the COM1 serial line only when a UART answered. The rest
 * of the kernel talks to this file and never to a device directly.
 */

#ifndef GENESIS_LIB_CONSOLE_H
#define GENESIS_LIB_CONSOLE_H

#include <stdint.h>

/* Bring up the VGA screen, probe the UART, and clear the screen. */
void console_init(void);

/* Non-zero when a serial UART was found and output is mirrored there. */
int console_serial_present(void);

/* Clear the screen and park the cursor at the top-left corner. */
void console_clear(void);

void console_putc(char c);
void console_puts(const char *text);

/* Numbers are formatted once, so both outputs always agree. */
void console_put_dec(uint32_t value);
void console_put_hex(uint32_t value);

#endif /* GENESIS_LIB_CONSOLE_H */
