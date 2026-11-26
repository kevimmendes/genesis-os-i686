/*
 * Project Genesis - kernel console.
 *
 * Fan-out layer between the kernel and its output devices: the VGA screen is
 * always available, the serial line is used only when the UART probe found real
 * hardware. Bits of formatting live here so both outputs print identical text.
 */

#include <arch/i686/serial.h>
#include <arch/i686/vga.h>
#include <lib/console.h>

static int console_serial;

void console_init(void)
{
    vga_init();
    serial_init();
    console_serial = serial_present();
}

int console_serial_present(void)
{
    return console_serial;
}

static void console_write_raw(const char *text)
{
    while (*text != '\0') {
        vga_putc(*text);
        if (console_serial != 0) {
            serial_putc(*text);
        }
        text++;
    }
}

void console_clear(void)
{
    static const char ansi_clear[] = "\033[2J\033[H";

    vga_init();

    /* An ANSI clear tidies a captured serial log, and must go only to the
     * serial line: escape bytes drawn on the VGA screen would be noise. */
    if (console_serial != 0) {
        for (const char *cursor = ansi_clear; *cursor != '\0'; cursor++) {
            serial_putc(*cursor);
        }
    }
}

void console_putc(char c)
{
    vga_putc(c);
    if (console_serial != 0) {
        serial_putc(c);
    }
}

void console_puts(const char *text)
{
    console_write_raw(text);
}

void console_put_dec(uint32_t value)
{
    char buffer[10];
    int index = 0;

    if (value == 0) {
        console_putc('0');
        return;
    }

    while (value != 0) {
        buffer[index++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (index > 0) {
        console_putc(buffer[--index]);
    }
}

void console_put_hex(uint32_t value)
{
    static const char digits[] = "0123456789abcdef";
    char buffer[8];
    int index = 0;

    console_puts("0x");

    if (value == 0) {
        console_putc('0');
        return;
    }

    while (value != 0) {
        buffer[index++] = digits[value & 0xF];
        value >>= 4;
    }

    while (index > 0) {
        console_putc(buffer[--index]);
    }
}
