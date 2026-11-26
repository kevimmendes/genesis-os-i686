/*
 * Project Genesis - boot and machine reports.
 *
 * The text the kernel prints about itself lives here instead of in main.c, so
 * the boot path and the shell's `info` command show exactly the same lines.
 */

#ifndef GENESIS_KERNEL_REPORT_H
#define GENESIS_KERNEL_REPORT_H

#include <arch/i686/boot_info.h>

void report_banner(void);

/* Protocol, loader and the memory the bootloader reported. */
void report_boot_info(const struct boot_info *info);

/* CPU, console, interrupt controller and frame allocator state. */
void report_machine(void);

#endif /* GENESIS_KERNEL_REPORT_H */
