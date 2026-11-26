/*
 * Project Genesis - 8259 PIC driver.
 *
 * Only the legacy two-chip configuration is supported: the reference laptop
 * has no APIC worth the complexity, and 16 IRQ lines are more than enough for
 * a timer, a keyboard and a disk.
 */

#include <arch/i686/io.h>
#include <arch/i686/pic.h>

#define PIC1_COMMAND 0x20u
#define PIC1_DATA    0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA    0xA1u

#define ICW1_INIT    0x10u
#define ICW1_ICW4    0x01u
#define ICW4_8086    0x01u

#define PIC_EOI      0x20u

#define CASCADE_IRQ  2u

/* Shadow copies of the interrupt masks, one bit per line. */
static uint8_t master_mask = 0xFFu;
static uint8_t slave_mask = 0xFFu;

void pic_remap(uint8_t master_offset, uint8_t slave_offset)
{
    /* Start the initialisation sequence on both chips. */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* Vector offsets: the whole point of the remap. */
    outb(PIC1_DATA, master_offset);
    io_wait();
    outb(PIC2_DATA, slave_offset);
    io_wait();

    /* Wiring: the slave is cascaded into master line 2. */
    outb(PIC1_DATA, (uint8_t)(1u << CASCADE_IRQ));
    io_wait();
    outb(PIC2_DATA, 0x02u);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Everything masked until a driver asks for its line. */
    master_mask = 0xFFu;
    slave_mask = 0xFFu;
    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

void pic_mask_irq(uint8_t irq)
{
    if (irq < 8) {
        master_mask |= (uint8_t)(1u << irq);
        outb(PIC1_DATA, master_mask);
    } else if (irq < 16) {
        slave_mask |= (uint8_t)(1u << (irq - 8u));
        outb(PIC2_DATA, slave_mask);
    }
}

void pic_unmask_irq(uint8_t irq)
{
    if (irq < 8) {
        master_mask &= (uint8_t)~(1u << irq);
        outb(PIC1_DATA, master_mask);
    } else if (irq < 16) {
        slave_mask &= (uint8_t)~(1u << (irq - 8u));
        outb(PIC2_DATA, slave_mask);

        /* The slave is only reachable through the cascade line. */
        master_mask &= (uint8_t)~(1u << CASCADE_IRQ);
        outb(PIC1_DATA, master_mask);
    }
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}
