/*
 * Project Genesis - CPU identification (i686).
 *
 * Prints the CPU vendor string from CPUID. On the reference laptop this is how
 * you confirm, from the kernel itself, that the machine really is the Pentium M
 * the specification describes.
 */

#ifndef GENESIS_ARCH_I686_CPU_H
#define GENESIS_ARCH_I686_CPU_H

/* 13-byte NUL-terminated vendor name, or "unknown" when CPUID is absent. */
const char *cpu_vendor(void);

#endif /* GENESIS_ARCH_I686_CPU_H */
