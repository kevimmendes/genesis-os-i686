/*
 * Project Genesis - CPU vendor identification.
 *
 * CPUID is not present on pre-486 processors, so the ID flag (bit 21 of
 * EFLAGS) is tested by flipping it: if the bit refuses to change, CPUID does
 * not exist and executing it would be an illegal instruction.
 */

#include <arch/i686/cpu.h>

#include <stdint.h>

static char vendor[13];

static int cpu_has_cpuid(void)
{
    uint32_t original;
    uint32_t flipped;

    __asm__ volatile("pushfl\n\t"
                     "popl %0\n\t"
                     "movl %0, %1\n\t"
                     "xorl $0x200000, %1\n\t"
                     "pushl %1\n\t"
                     "popfl\n\t"
                     "pushfl\n\t"
                     "popl %1"
                     : "=&r"(original), "=&r"(flipped)
                     :
                     : "cc");

    /* Put the caller'\''s flags back. */
    __asm__ volatile("pushl %0\n\tpopfl" : : "r"(original) : "cc");

    return ((original ^ flipped) & 0x200000u) != 0;
}

static void cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                  uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf));
}

const char *cpu_vendor(void)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    if (cpu_has_cpuid() == 0) {
        return "unknown";
    }

    cpuid(0, &eax, &ebx, &ecx, &edx);

    /* The vendor string is split across EBX, EDX and ECX, in that order. */
    const uint32_t words[3] = { ebx, edx, ecx };

    for (uint32_t i = 0; i < 3; i++) {
        for (uint32_t j = 0; j < 4; j++) {
            vendor[i * 4 + j] = (char)((words[i] >> (j * 8)) & 0xFF);
        }
    }
    vendor[12] = '\''\0'\'';

    return vendor;
}
