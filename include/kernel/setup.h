/*
 * Project Genesis - first-boot setup wizard.
 *
 * The ISO offers two GRUB entries: a live session and "Install Genesis". The
 * second one passes `setup` on the kernel command line, which is what brings
 * this wizard up before the shell.
 *
 * The wizard collects the three choices every installer asks for - language,
 * keyboard layout and how much disk space the system may use - and applies
 * what this version can apply. The disk choice is recorded but nothing is
 * written yet: the kernel has no ATA driver and no filesystem (milestone M8),
 * and pretending otherwise would risk somebody's real disk.
 */

#ifndef GENESIS_KERNEL_SETUP_H
#define GENESIS_KERNEL_SETUP_H

#include <stdint.h>

#define SETUP_LANGUAGE_PT 0
#define SETUP_LANGUAGE_EN 1
#define SETUP_LANGUAGE_ES 2

/* Keyboard layouts understood by the PS/2 driver. */
#define SETUP_KEYBOARD_US 0
#define SETUP_KEYBOARD_PT 1
#define SETUP_KEYBOARD_ES 2

struct setup_config {
    int language;
    int keyboard;
    uint32_t disk_gb;
};

/* Non-zero when the kernel command line asks for the wizard ("setup"). */
int setup_requested(const char *cmdline);

/* Run the interactive wizard. Interrupts and the keyboard must already work. */
void setup_run(void);

/* The choices made, or the defaults when the wizard did not run. */
const struct setup_config *setup_config_get(void);

#endif /* GENESIS_KERNEL_SETUP_H */
