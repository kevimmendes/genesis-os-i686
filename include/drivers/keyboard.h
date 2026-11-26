/*
 * Project Genesis - PS/2 keyboard (i686, scan code set 1).
 *
 * The 8042 controller delivers one scan code per byte on port 0x60 and raises
 * IRQ1. The driver translates scan codes to ASCII (letters, digits, punctuation
 * and a few symbols, with Shift) and hands characters to the shell.
 *
 * Scan codes that map to nothing - function keys, arrows, releases - are
 * dropped instead of being guessed at.
 */

#ifndef GENESIS_DRIVERS_KEYBOARD_H
#define GENESIS_DRIVERS_KEYBOARD_H

/* Register the IRQ1 handler and unmask the line. */
void keyboard_init(void);

/* Select 0 = US, 1 = PT, 2 = ES. Only the keys whose symbols differ are
 * remapped, and accented letters fall back to their base letter (c, n) because
 * the console is ASCII-only for now. */
void keyboard_set_layout(int layout);
int keyboard_layout(void);

/* Wait for one character. Returns '\n' for Enter and '\b' for Backspace.
 * Blocks with `hlt` between polls, so an idle prompt costs no CPU. */
char keyboard_getchar(void);

#endif /* GENESIS_DRIVERS_KEYBOARD_H */
