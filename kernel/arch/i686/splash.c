/*
 * Project Genesis - boot splash screen.
 *
 * Two jobs, both deliberately primitive:
 *
 *   1. Paint the 80x24 art over the 80x25 text buffer at 0xB8000.
 *   2. Hold it for a moment so it can be seen, then return.
 *
 * The wait uses the 8253/8254 programmable interval timer, channel 2, whose
 * output state is readable as bit 5 of port 0x61. That needs no IDT and no
 * interrupts, which is what makes it usable this early. The speaker gate bits
 * of port 0x61 stay clear, so nothing is ever heard.
 */

#include <arch/i686/io.h>
#include <arch/i686/splash.h>

#define VGA_TEXT_BASE 0xB8000u
#define VGA_COLUMNS   80

/* Light grey on black. Try 0x0A for a green phosphor look, 0x0E for amber. */
#define SPLASH_ATTR   0x07u

#define CRTC_INDEX    0x3D4
#define CRTC_DATA     0x3D5

#define PIT_CHANNEL2  0x42
#define PIT_COMMAND   0x43
#define SPEAKER_PORT  0x61

/* 8254 mode 3 (square wave) on channel 2 with divisor 0 (= 65536) gives a
 * period of 65536 / 1.193182 MHz = 54.9 ms. The output bit changes once per
 * half period, so one observed change is a wait of about 27.5 ms. */
#define PIT_FULL_PERIOD_MS 55u

/* Real port I/O costs about a microsecond per read, so a bit that does not
 * change within this many polls means the timer is not answering and the
 * splash must give up instead of stalling the boot. */
#define PIT_POLL_LIMIT 200000u

static volatile uint16_t *const vga = (volatile uint16_t *)VGA_TEXT_BASE;

void splash_show(void)
{
    /* Hide the hardware cursor so it does not blink over the picture. */
    outb(CRTC_INDEX, 0x0A);
    outb(CRTC_DATA, 0x20);

    for (uint32_t column = 0; column < VGA_COLUMNS; column++) {
        vga[column] = (uint16_t)' ' | (uint16_t)(SPLASH_ATTR << 8);
    }

    for (uint32_t row = 0; row < SPLASH_ART_ROWS; row++) {
        uint32_t screen_row = row + 1;

        for (uint32_t column = 0; column < SPLASH_ART_COLUMNS; column++) {
            char glyph = splash_art[row][column];

            vga[screen_row * VGA_COLUMNS + column] =
                (uint16_t)(uint8_t)glyph | (uint16_t)(SPLASH_ATTR << 8);
        }
    }
}

void splash_hold_ms(uint32_t milliseconds)
{
    uint32_t changes_needed =
        (milliseconds * 2u + PIT_FULL_PERIOD_MS / 2u) / PIT_FULL_PERIOD_MS;
    uint32_t changes = 0;
    uint32_t polls = 0;
    uint8_t previous;

    /* Speaker off: only the timer output bit is read, never the gate bits. */
    outb(SPEAKER_PORT, (uint8_t)(inb(SPEAKER_PORT) & ~0x03u));

    outb(PIT_COMMAND, 0xB6);    /* channel 2, lobyte/hibyte, mode 3, binary */
    outb(PIT_CHANNEL2, 0x00);
    outb(PIT_CHANNEL2, 0x00);   /* divisor 0 means 65536 */

    previous = (uint8_t)(inb(SPEAKER_PORT) & 0x20u);

    while (changes < changes_needed) {
        uint8_t current = (uint8_t)(inb(SPEAKER_PORT) & 0x20u);

        if (current != previous) {
            previous = current;
            changes++;
            polls = 0;
        } else if (++polls > PIT_POLL_LIMIT) {
            break;              /* no timer answering: keep booting */
        }
    }
}
