/*
 * Project Genesis - Interrupt Descriptor Table setup and dispatch.
 *
 * Every gate uses the kernel code selector (0x08) and is a 32-bit interrupt
 * gate (0x8E: present, ring 0, interrupt). Exceptions and IRQs are handled in
 * C; the assembly stubs only save the machine state and call
 * interrupt_dispatch().
 */

#include <arch/i686/idt.h>
#include <arch/i686/pic.h>
#include <kernel/panic.h>

/* One 8-byte gate, in the layout the CPU requires. */
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attributes;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define KERNEL_CODE_SELECTOR 0x08u
#define GATE_INTERRUPT_32    0x8Eu

/* Addresses of the assembly stubs, defined in kernel/arch/i686/interrupts.s. */
extern const uint32_t isr_stub_table[32];
extern const uint32_t irq_stub_table[IRQ_COUNT];

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_pointer idt_register;
static irq_handler_t irq_handlers[IRQ_COUNT];
static volatile uint32_t interrupt_total;

static const char *const exception_names[32] = {
    "divide error (#DE)",
    "debug (#DB)",
    "non-maskable interrupt",
    "breakpoint (#BP)",
    "overflow (#OF)",
    "bound range exceeded (#BR)",
    "invalid opcode (#UD)",
    "device not available (#NM)",
    "double fault (#DF)",
    "coprocessor segment overrun",
    "invalid TSS (#TS)",
    "segment not present (#NP)",
    "stack-segment fault (#SS)",
    "general protection fault (#GP)",
    "page fault (#PF)",
    "reserved (15)",
    "x87 floating-point error (#MF)",
    "alignment check (#AC)",
    "machine check (#MC)",
    "SIMD floating-point error (#XM)",
    "virtualisation exception (#VE)",
    "control protection (#CP)",
    "reserved (22)",
    "reserved (23)",
    "reserved (24)",
    "reserved (25)",
    "reserved (26)",
    "reserved (27)",
    "reserved (28)",
    "reserved (29)",
    "reserved (30)",
    "reserved (31)",
};

static void idt_set_gate(uint8_t vector, uint32_t handler)
{
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFFu);
    idt[vector].selector = KERNEL_CODE_SELECTOR;
    idt[vector].zero = 0;
    idt[vector].type_attributes = GATE_INTERRUPT_32;
    idt[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFFu);
}

void idt_init(void)
{
    for (uint32_t vector = 0; vector < IDT_ENTRIES; vector++) {
        idt_set_gate((uint8_t)vector, 0);
    }

    for (uint32_t vector = 0; vector < 32; vector++) {
        idt_set_gate((uint8_t)vector, isr_stub_table[vector]);
    }

    for (uint32_t irq = 0; irq < IRQ_COUNT; irq++) {
        idt_set_gate((uint8_t)(32u + irq), irq_stub_table[irq]);
        irq_handlers[irq] = 0;
    }

    idt_register.limit = (uint16_t)(sizeof(idt) - 1u);
    idt_register.base = (uint32_t)(uintptr_t)idt;
    __asm__ volatile("lidt %0" : : "m"(idt_register));
}

void irq_install(uint8_t irq, irq_handler_t handler)
{
    if (irq < IRQ_COUNT) {
        irq_handlers[irq] = handler;
    }
}

uint32_t interrupts_handled(void)
{
    return interrupt_total;
}

void interrupt_dispatch(struct interrupt_frame *frame)
{
    uint32_t vector = frame->vector;

    interrupt_total++;

    if (vector < 32) {
        exception_report(exception_names[vector], frame);
    }

    if (vector >= 32 && vector < 32 + IRQ_COUNT) {
        uint32_t irq = vector - 32;

        if (irq_handlers[irq] != 0) {
            irq_handlers[irq](frame);
        }

        /* Acknowledge the interrupt controller, otherwise no further IRQ of
         * this or lower priority is ever delivered again. */
        pic_send_eoi((uint8_t)irq);
    }
}
