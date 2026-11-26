/*
 * Project Genesis - interactive shell.
 *
 * A line editor (echo, backspace), a small command table and nothing else. All
 * output goes through the console layer, so whatever appears on the laptop's
 * screen also appears on a serial capture.
 */

#include <arch/i686/io.h>
#include <arch/i686/pit.h>
#include <arch/i686/splash.h>
#include <drivers/keyboard.h>
#ifdef GENESIS_ENABLE_AGENT
#include <kernel/agent.h>
#endif
#include <kernel/report.h>
#include <kernel/shell.h>
#include <lib/console.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/pmm.h>

#define LINE_MAX 128u

static const struct boot_info *boot;
static char line[LINE_MAX];

static void shell_halt(void) __attribute__((noreturn));
static void shell_reboot(void) __attribute__((noreturn));

static void print_help(void)
{
    console_puts("Commands:\n");
    console_puts("  help           show this list\n");
    console_puts("  clear          clear the screen\n");
    console_puts("  info           boot and hardware report\n");
    console_puts("  mem            physical frames and kernel heap\n");
    console_puts("  uptime         time since the timer started\n");
    console_puts("  calc <a> <op> <b>  integer arithmetic (+ - * / %)\n");
    console_puts("  hexdump [addr] [n] dump memory bytes (default 0x100000, 64)\n");
#ifdef GENESIS_ENABLE_AGENT
    console_puts("  ai <question>  ask the host model over the serial link\n");
#endif
    console_puts("  echo <text>    print the text back\n");
    console_puts("  splash         draw the boot art again\n");
    console_puts("  reboot         restart the machine\n");
    console_puts("  halt           stop the CPU\n");
}

static void print_memory(void)
{
    console_puts("frames     : ");
    console_put_dec(pmm_free_frames());
    console_puts(" free of ");
    console_put_dec(pmm_total_frames());
    console_puts(" frames of 4 KiB (");
    console_put_dec(pmm_used_frames());
    console_puts(" in use)\n");

    console_puts("managed    : ");
    console_put_dec(pmm_total_kib());
    console_puts(" KiB\n");

    console_puts("heap       : ");
    console_put_dec(heap_used());
    console_puts(" bytes handed out (bump allocator, memory is never reused)\n");
}

static void print_uptime(void)
{
    console_puts("uptime     : ");
    console_put_dec(timer_seconds());
    console_puts(" s (");
    console_put_dec(timer_ticks());
    console_puts(" ticks at ");
    console_put_dec(timer_hz());
    console_puts(" Hz)\n");
}

/* Parse a decimal number, or hexadecimal with a 0x prefix. */
static uint32_t parse_number(const char *text, const char **end, int *ok)
{
    const char *cursor = text;
    uint32_t base = 10;
    uint32_t value = 0;
    uint32_t digits = 0;

    *ok = 0;

    if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X')) {
        base = 16;
        cursor += 2;
    }

    for (;;) {
        char c = *cursor;
        uint32_t digit;

        if (c >= '0' && c <= '9') {
            digit = (uint32_t)(c - '0');
        } else if (base == 16 && c >= 'a' && c <= 'f') {
            digit = (uint32_t)(c - 'a') + 10u;
        } else if (base == 16 && c >= 'A' && c <= 'F') {
            digit = (uint32_t)(c - 'A') + 10u;
        } else {
            break;
        }

        value = value * base + digit;
        cursor++;
        digits++;
    }

    if (digits == 0) {
        return 0;
    }

    *ok = 1;
    if (end != 0) {
        *end = cursor;
    }

    return value;
}

static void print_address_hex(uint32_t value)
{
    static const char digits[] = "0123456789abcdef";

    console_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        console_putc(digits[(value >> shift) & 0xFu]);
    }
}

static void print_byte_hex(uint8_t value)
{
    static const char digits[] = "0123456789abcdef";

    console_putc(digits[(value >> 4) & 0xFu]);
    console_putc(digits[value & 0xFu]);
}

static void print_calc(const char *args)
{
    const char *cursor = args;
    uint32_t left;
    uint32_t right;
    uint32_t result;
    char op;
    int ok;

    left = parse_number(cursor, &cursor, &ok);
    while (*cursor == ' ') {
        cursor++;
    }
    if (ok == 0 || *cursor == '\0') {
        console_puts("usage: calc <number> <op> <number>  (op: + - * / %)\n");
        return;
    }

    op = *cursor++;
    while (*cursor == ' ') {
        cursor++;
    }

    right = parse_number(cursor, &cursor, &ok);
    while (*cursor == ' ') {
        cursor++;
    }
    if (ok == 0 || *cursor != '\0') {
        console_puts("usage: calc <number> <op> <number>  (op: + - * / %)\n");
        return;
    }

    switch (op) {
    case '+':
        result = left + right;
        break;
    case '-':
        result = left - right;
        break;
    case '*':
        result = left * right;
        break;
    case '/':
    case '%':
        if (right == 0) {
            console_puts("calc: division by zero\n");
            return;
        }
        result = (op == '/') ? left / right : left % right;
        break;
    default:
        console_puts("calc: unknown operator (use + - * / %)\n");
        return;
    }

    console_put_dec(left);
    console_puts(" ");
    console_putc(op);
    console_puts(" ");
    console_put_dec(right);
    console_puts(" = ");
    console_put_dec(result);
    console_puts("\n");
}

#define HEXDUMP_DEFAULT_ADDRESS 0x00100000u
#define HEXDUMP_DEFAULT_LENGTH  64u
#define HEXDUMP_MAX_LENGTH      256u

static void print_hexdump(const char *args)
{
    const char *cursor = args;
    const uint8_t *bytes;
    uint32_t address = HEXDUMP_DEFAULT_ADDRESS;
    uint32_t length = HEXDUMP_DEFAULT_LENGTH;
    int ok = 1;

    if (*cursor != '\0') {
        address = parse_number(cursor, &cursor, &ok);
        while (*cursor == ' ') {
            cursor++;
        }
        if (ok == 0) {
            console_puts("usage: hexdump [address] [length]  (max 256 bytes)\n");
            return;
        }
        if (*cursor != '\0') {
            length = parse_number(cursor, &cursor, &ok);
            if (ok == 0) {
                console_puts("usage: hexdump [address] [length]  (max 256 bytes)\n");
                return;
            }
        }
    }

    if (length == 0 || length > HEXDUMP_MAX_LENGTH) {
        console_puts("hexdump: length must be 1..256\n");
        return;
    }

    bytes = (const uint8_t *)(uintptr_t)address;

    for (uint32_t offset = 0; offset < length; offset += 16u) {
        uint32_t row = length - offset < 16u ? length - offset : 16u;

        print_address_hex(address + offset);
        console_puts(": ");

        for (uint32_t column = 0; column < 16u; column++) {
            if (column < row) {
                print_byte_hex(bytes[offset + column]);
                console_putc(' ');
            } else {
                console_puts("   ");
            }
            if (column == 7u) {
                console_putc(' ');
            }
        }

        console_puts(" |");
        for (uint32_t column = 0; column < row; column++) {
            uint8_t value = bytes[offset + column];

            console_putc(value >= 32u && value < 127u ? (char)value : '.');
        }
        console_puts("|\n");
    }
}

static void shell_halt(void)
{
    console_puts("system halted. It is safe to power off.\n");
    cpu_disable_interrupts();

    for (;;) {
        cpu_halt();
    }
}

static void shell_reboot(void)
{
    console_puts("rebooting...\n");

    /* The 8042 keyboard controller can pulse the CPU reset line; every PC
     * since the AT has it, including the reference laptop. */
    outb(0x64, 0xFE);

    /* If that did not take, stop instead of executing random code. */
    for (;;) {
        cpu_halt();
    }
}

/* True when `text` starts with `word` and is followed by a space or the end. */
static int starts_with_word(const char *text, const char *word)
{
    size_t length = strlen(word);

    if (strncmp(text, word, length) != 0) {
        return 0;
    }

    return text[length] == '\0' || text[length] == ' ';
}

static const char *arguments_of(const char *text, const char *word)
{
    const char *cursor = text + strlen(word);

    while (*cursor == ' ') {
        cursor++;
    }

    return cursor;
}

static void execute_line(char *text)
{
    while (*text == ' ') {
        text++;
    }

    if (*text == '\0') {
        return;
    }

    if (strcmp(text, "help") == 0) {
        print_help();
    } else if (strcmp(text, "clear") == 0) {
        console_clear();
    } else if (strcmp(text, "info") == 0) {
        report_boot_info(boot);
        report_machine();
    } else if (strcmp(text, "mem") == 0) {
        print_memory();
    } else if (strcmp(text, "uptime") == 0) {
        print_uptime();
    } else if (starts_with_word(text, "calc")) {
        print_calc(arguments_of(text, "calc"));
    } else if (starts_with_word(text, "hexdump")) {
        print_hexdump(arguments_of(text, "hexdump"));
#ifdef GENESIS_ENABLE_AGENT
    } else if (starts_with_word(text, "ai")) {
        const char *prompt = arguments_of(text, "ai");

        if (*prompt == '\0') {
            console_puts("usage: ai <question>\n");
        } else {
            agent_ask(prompt);
        }
#endif
    } else if (starts_with_word(text, "echo")) {
        console_puts(arguments_of(text, "echo"));
        console_puts("\n");
    } else if (strcmp(text, "splash") == 0) {
        splash_show();
        splash_hold_ms(1500);
        console_clear();
    } else if (strcmp(text, "reboot") == 0) {
        shell_reboot();
    } else if (strcmp(text, "halt") == 0) {
        shell_halt();
    } else {
        console_puts("unknown command: ");
        console_puts(text);
        console_puts("\ntype 'help' for the list of commands.\n");
    }
}

/* Read one line, echoing it and handling Backspace. */
static void read_line(void)
{
    uint32_t length = 0;

    line[0] = '\0';

    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            console_putc('\n');
            line[length] = '\0';
            return;
        }

        if (c == '\b') {
            if (length > 0) {
                length--;
                console_puts("\b \b");
            }
            continue;
        }

        if (c < 32 || c > 126) {
            continue;
        }

        if (length + 1u >= LINE_MAX) {
            continue;
        }

        line[length++] = c;
        console_putc(c);
    }
}

void shell_run(const struct boot_info *info)
{
    boot = info;

    console_puts("\ntype 'help' and press Enter for the command list.\n");

    for (;;) {
        console_puts("genesis> ");
        read_line();
        execute_line(line);
    }
}
