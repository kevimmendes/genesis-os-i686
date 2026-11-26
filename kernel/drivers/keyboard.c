/*
 * Project Genesis - PS/2 keyboard driver.
 *
 * Characters arrive through IRQ1 into a small ring buffer. The shell also polls
 * the controller when it waits for input: that keeps the driver usable in
 * environments where the interrupt is not delivered (QEMU's loader path always
 * delivers it; the in-process Unicorn harness does not emulate the PIC) and it
 * means a lost interrupt can never leave the prompt deaf.
 *
 * The controller is never flushed at init: a byte the firmware left behind can
 * at worst produce one stray character, while flushing could throw away a
 * key pressed during the boot splash.
 */

#include <arch/i686/idt.h>
#include <arch/i686/io.h>
#include <arch/i686/pic.h>
#include <drivers/keyboard.h>

#define KEYBOARD_DATA   0x60u
#define KEYBOARD_STATUS 0x64u
#define STATUS_OUTPUT_FULL 0x01u

#define BUFFER_SIZE 64u

/* Scan code set 1 (the set every PC BIOS leaves the controller in). */
#define SCANCODE_LEFT_SHIFT   0x2Au
#define SCANCODE_RIGHT_SHIFT  0x36u
#define SCANCODE_LEFT_SHIFT_RELEASE  0xAAu
#define SCANCODE_RIGHT_SHIFT_RELEASE 0xB6u
#define SCANCODE_ENTER        0x1Cu
#define SCANCODE_BACKSPACE    0x0Eu

static const char keymap_unshifted[0x3A] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=',
    [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
    [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm',
    [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x39] = ' ',
};

static const char keymap_shifted[0x3A] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+',
    [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
    [0x27] = ':', [0x28] = '"', [0x29] = '~',
    [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B',
    [0x31] = 'N', [0x32] = 'M',
    [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x39] = ' ',
};

/* Keys whose symbols differ from the US table. Only ASCII is available, so
 * accented letters fall back to their base letter. */
struct layout_override {
    uint8_t scancode;
    char unshifted;
    char shifted;
};

static const struct layout_override layout_pt[] = {
    {0x0C, '\'', '"'},
    {0x0D, '<', '>'},
    {0x1A, '+', '*'},
    {0x1B, '`', '~'},
    {0x27, 'c', 'C'},   /* cedilla */
    {0x28, 'o', 'O'},   /* ordinal */
};

static const struct layout_override layout_es[] = {
    {0x0C, '\'', '?'},
    {0x0D, '!', '?'},
    {0x1A, '`', '^'},
    {0x1B, '+', '*'},
    {0x27, 'n', 'N'},   /* enye */
    {0x28, 'o', 'O'},
};

static char ring[BUFFER_SIZE];
static uint32_t ring_head;
static uint32_t ring_tail;
static int shift_held;
static int current_layout;

static void ring_push(char c)
{
    uint32_t next = (ring_head + 1u) % BUFFER_SIZE;

    if (next != ring_tail) {
        ring[ring_head] = c;
        ring_head = next;
    }
}

static int ring_pop(char *out)
{
    if (ring_head == ring_tail) {
        return 0;
    }

    *out = ring[ring_tail];
    ring_tail = (ring_tail + 1u) % BUFFER_SIZE;
    return 1;
}

static char translate(uint8_t scancode)
{
    if (scancode == SCANCODE_LEFT_SHIFT || scancode == SCANCODE_RIGHT_SHIFT) {
        shift_held = 1;
        return 0;
    }

    if (scancode == SCANCODE_LEFT_SHIFT_RELEASE ||
        scancode == SCANCODE_RIGHT_SHIFT_RELEASE) {
        shift_held = 0;
        return 0;
    }

    if ((scancode & 0x80u) != 0 || scancode >= 0x3Au) {
        return 0;
    }

    char base = shift_held ? keymap_shifted[scancode] : keymap_unshifted[scancode];
    const struct layout_override *table = 0;
    uint32_t count = 0;

    if (current_layout == 1) {
        table = layout_pt;
        count = (uint32_t)(sizeof(layout_pt) / sizeof(layout_pt[0]));
    } else if (current_layout == 2) {
        table = layout_es;
        count = (uint32_t)(sizeof(layout_es) / sizeof(layout_es[0]));
    }

    for (uint32_t index = 0; index < count; index++) {
        if (table[index].scancode == scancode) {
            return shift_held ? table[index].shifted : table[index].unshifted;
        }
    }

    return base;
}

void keyboard_set_layout(int layout)
{
    if (layout >= 0 && layout <= 2) {
        current_layout = layout;
    }
}

int keyboard_layout(void)
{
    return current_layout;
}

static void keyboard_on_irq(struct interrupt_frame *frame)
{
    char c;

    (void)frame;

    c = translate(inb(KEYBOARD_DATA));
    if (c != 0) {
        ring_push(c);
    }
}

/* Pop a buffered character, or ask the controller directly. */
static int keyboard_poll(char *out)
{
    uint8_t scancode;
    char c;

    if (ring_pop(out)) {
        return 1;
    }

    if ((inb(KEYBOARD_STATUS) & STATUS_OUTPUT_FULL) == 0) {
        return 0;
    }

    scancode = inb(KEYBOARD_DATA);
    c = translate(scancode);
    if (c == 0) {
        return 0;
    }

    *out = c;
    return 1;
}

void keyboard_init(void)
{
    ring_head = 0;
    ring_tail = 0;
    shift_held = 0;
    current_layout = 0;

    irq_install(1, keyboard_on_irq);
    pic_unmask_irq(1);
}

char keyboard_getchar(void)
{
    char c;

    for (;;) {
        if (keyboard_poll(&c)) {
            return c;
        }

        /* Interrupts are on here: this sleeps until the timer or a key wakes
         * the CPU, so an idle prompt does not spin. */
        cpu_idle();
    }
}
