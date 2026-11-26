/*
 * Project Genesis - VGA text console driver (80x25, colour 7 on black).
 *
 * The screen buffer is a flat array of 16-bit cells at 0xB8000: the low byte is
 * the character, the high byte is the attribute. Scrolling copies the whole
 * screen up one line rather than using hardware scroll registers, which keeps
 * the code short and portable across every VGA-compatible chip (the reference
 * machine's Radeon 9000 included).
 */

#include <arch/i686/io.h>
#include <arch/i686/vga.h>

#define VGA_TEXT_BASE 0xB8000u
#define VGA_ATTR      0x07u /* light grey on black */

/* VGA CRTC registers, used to place the blinking hardware cursor. */
#define CRTC_INDEX 0x3D4
#define CRTC_DATA  0x3D5

static volatile uint16_t *const vga_cells = (volatile uint16_t *)VGA_TEXT_BASE;
static uint32_t vga_row;
static uint32_t vga_column;

static uint16_t vga_cell(char c)
{
    return (uint16_t)(uint8_t)c | (uint16_t)(VGA_ATTR << 8);
}

static void vga_move_cursor(void)
{
    uint16_t position = (uint16_t)(vga_row * VGA_WIDTH + vga_column);

    outb(CRTC_INDEX, 0x0F);            /* cursor location low byte  */
    outb(CRTC_DATA, (uint8_t)(position & 0xFF));
    outb(CRTC_INDEX, 0x0E);            /* cursor location high byte */
    outb(CRTC_DATA, (uint8_t)(position >> 8));
}

static void vga_scroll(void)
{
    uint32_t column;

    for (uint32_t row = 1; row < VGA_HEIGHT; row++) {
        for (column = 0; column < VGA_WIDTH; column++) {
            vga_cells[(row - 1) * VGA_WIDTH + column] =
                vga_cells[row * VGA_WIDTH + column];
        }
    }

    for (column = 0; column < VGA_WIDTH; column++) {
        vga_cells[(VGA_HEIGHT - 1) * VGA_WIDTH + column] = vga_cell(' ');
    }
}

void vga_init(void)
{
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_cells[i] = vga_cell(' ');
    }

    /* Undo a hidden cursor (the splash hides it): a normal underline. */
    outb(CRTC_INDEX, 0x0A);            /* cursor start scanline */
    outb(CRTC_DATA, 0x0E);
    outb(CRTC_INDEX, 0x0B);            /* cursor end scanline   */
    outb(CRTC_DATA, 0x0F);

    vga_row = 0;
    vga_column = 0;
    vga_move_cursor();
}

void vga_putc(char c)
{
    if (c == '\n') {
        vga_column = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_column = 0;
    } else if (c == '\t') {
        /* Advance to the next multiple of four columns. */
        vga_column = (vga_column + 4u) & ~3u;
    } else {
        vga_cells[vga_row * VGA_WIDTH + vga_column] = vga_cell(c);
        vga_column++;
    }

    if (vga_column >= VGA_WIDTH) {
        vga_column = 0;
        vga_row++;
    }

    while (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row--;
    }

    vga_move_cursor();
}
