/*
 * Project Genesis - Interrupt Descriptor Table (i686).
 *
 * The IDT tells the CPU where to jump for each of the 256 interrupt vectors.
 * Vectors 0-31 are CPU exceptions, 32-47 are the hardware IRQs after the 8259
 * PIC has been remapped away from the exception range.
 *
 * `struct interrupt_frame` is the exact stack layout built by the assembly
 * stubs in kernel/arch/i686/interrupts.s: the registers saved by `pusha`,
 * followed by the vector, the error code and the frame the CPU itself pushed.
 */

#ifndef GENESIS_ARCH_I686_IDT_H
#define GENESIS_ARCH_I686_IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256
#define IRQ_COUNT   16

struct interrupt_frame {
    /* Saved by pusha, lowest address first. */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* Pushed by the stub (or by the CPU for exceptions with an error code). */
    uint32_t vector;
    uint32_t error;

    /* Pushed by the CPU when the interrupt was taken. */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

typedef void (*irq_handler_t)(struct interrupt_frame *frame);

/* Install all 256 gates and load the IDT. Interrupts stay disabled. */
void idt_init(void);

/* Register a C handler for one IRQ line (0-15). Replaces any previous one. */
void irq_install(uint8_t irq, irq_handler_t handler);

/* Number of interrupts dispatched since boot, exceptions included. */
uint32_t interrupts_handled(void);

/* Called from the assembly stubs; not meant to be called from C code. */
void interrupt_dispatch(struct interrupt_frame *frame);

#endif /* GENESIS_ARCH_I686_IDT_H */
