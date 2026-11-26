/*
 * Project Genesis - interactive kernel shell.
 *
 * The shell reads lines from the PS/2 keyboard and runs built-in commands. It
 * runs in ring 0 on the boot thread: there is no scheduler and no user mode
 * yet, so this is the whole user interface of the system.
 */

#ifndef GENESIS_KERNEL_SHELL_H
#define GENESIS_KERNEL_SHELL_H

#include <arch/i686/boot_info.h>

/* Print the prompt and never return: the shell owns the machine from here. */
void shell_run(const struct boot_info *info) __attribute__((noreturn));

#endif /* GENESIS_KERNEL_SHELL_H */
