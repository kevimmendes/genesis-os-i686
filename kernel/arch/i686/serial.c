/*
 * Project Genesis - COM1 serial console driver (16550 UART).
 *
 * Three things matter:
 *
 *   1. The reference laptop has a 16550-compatible 9-pin serial port, so this
 *      driver is also the link to a host PC. See docs/AGENT.md.
 *   2. Writing to an absent UART can hang a naive driver forever waiting for
 *      the transmit buffer, so serial_init() probes with the scratch register
 *      and the writers give up after a bounded wait.
 *   3. Nothing may be lost silently: if the probe fails, serial_present()
 *      returns 0 and the console simply stays on the screen.
 */

#include <arch/i686/io.h>
#include <arch/i686/serial.h>

/* 16550 register offsets from the base I/O port. */
#define UART_DATA        0 /* also divisor low when DLAB = 1 */
#define UART_IER         1 /* also divisor high when DLAB = 1 */
#define UART_FIFO_CTRL   2
#define UART_LINE_CTRL   3
#define UART_MODEM_CTRL  4
#define UART_LINE_STATUS 5
#define UART_SCRATCH     7

#define UART_LSR_DATA_READY 0x01
#define UART_LSR_THR_EMPTY  0x20
#define UART_LCR_DLAB       0x80
#define UART_LCR_8N1        0x03

/* Divisor 1 -> 115200 baud; the QEMU default and accepted by every real UART. */
#define UART_DIVISOR_LOW    0x01
#define UART_DIVISOR_HIGH   0x00

/* Writes are attempted at most this many times before being dropped. */
#define UART_TX_SPIN_LIMIT  100000u

static int uart_present;

void serial_init(void)
{
    uart_present = 0;

    outb(SERIAL_COM1 + UART_IER, 0x00);    /* no interrupts: we poll */
    outb(SERIAL_COM1 + UART_LINE_CTRL, UART_LCR_DLAB);
    outb(SERIAL_COM1 + UART_DATA, UART_DIVISOR_LOW);
    outb(SERIAL_COM1 + UART_IER, UART_DIVISOR_HIGH);
    outb(SERIAL_COM1 + UART_LINE_CTRL, UART_LCR_8N1);
    outb(SERIAL_COM1 + UART_FIFO_CTRL, 0xC7); /* enable and clear FIFOs */
    outb(SERIAL_COM1 + UART_MODEM_CTRL, 0x03);

    /*
     * Probe: the scratch register exists on the 16550 and holds any byte. On a
     * machine with no UART at all the bus usually reads back 0xFF, so the
     * pattern check is enough to tell "device" from "empty port".
     */
    outb(SERIAL_COM1 + UART_SCRATCH, 0x5A);
    if (inb(SERIAL_COM1 + UART_SCRATCH) == 0x5A) {
        uart_present = 1;
    }
}

int serial_present(void)
{
    return uart_present;
}

int serial_try_getc(char *out)
{
    if (uart_present == 0) {
        return 0;
    }

    if ((inb(SERIAL_COM1 + UART_LINE_STATUS) & UART_LSR_DATA_READY) == 0) {
        return 0;
    }

    *out = (char)inb(SERIAL_COM1 + UART_DATA);
    return 1;
}

void serial_putc(char c)
{
    if (uart_present == 0) {
        return;
    }

    /* A bare newline only moves the cursor down on a terminal, so send CR too. */
    if (c == '\''\n'\'') {
        serial_putc('\''\r'\'');
    }

    for (uint32_t spin = 0; spin < UART_TX_SPIN_LIMIT; spin++) {
        if ((inb(SERIAL_COM1 + UART_LINE_STATUS) & UART_LSR_THR_EMPTY) != 0) {
            outb(SERIAL_COM1 + UART_DATA, (uint8_t)c);
            return;
        }
    }

    /* Transmitter stuck: drop the byte rather than freeze the kernel. */
}
