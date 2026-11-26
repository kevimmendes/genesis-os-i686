/*
 * Project Genesis - serial console (i686).
 *
 * COM1 is used as a second diagnostic channel: QEMU can capture it directly
 * (-serial stdio), which makes automated boot tests possible without reading
 * the screen. On the reference laptop there is no serial port at all, so
 * serial_init() probes for the UART and every writer checks serial_present().
 */

#ifndef GENESIS_ARCH_I686_SERIAL_H
#define GENESIS_ARCH_I686_SERIAL_H

#include <stdint.h>

#define SERIAL_COM1 0x3F8

/* Probe COM1 and configure it for 115200 8N1. Safe to call on a machine
 * without a UART: it then reports absence instead of touching the port. */
void serial_init(void);

/* Non-zero when a working UART answered during serial_init(). */
int serial_present(void);

/* Blocking transmit of one character. No-op when no UART was found, and the
 * internal wait is bounded so dead hardware can never freeze the kernel. */
void serial_putc(char c);

/* Read one received character if the UART has one. Returns 1 and stores the
 * byte, or 0 when nothing arrived. Never blocks. */
int serial_try_getc(char *out);

#endif /* GENESIS_ARCH_I686_SERIAL_H */
