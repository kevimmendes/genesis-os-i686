/*
 * Project Genesis - 8259 programmable interrupt controller (i686).
 *
 * Two cascaded 8259A chips deliver the 16 legacy IRQ lines. Out of reset they
 * map IRQ0-7 onto vectors 0x08-0x0F, which collide with CPU exceptions, so the
 * kernel remaps them to 32-47 before enabling anything.
 */

#ifndef GENESIS_ARCH_I686_PIC_H
#define GENESIS_ARCH_I686_PIC_H

#include <stdint.h>

/* Remap the master to `master_offset` and the slave to `slave_offset`, then
 * mask every line. Unmask individual lines with pic_unmask_irq(). */
void pic_remap(uint8_t master_offset, uint8_t slave_offset);

void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);

/* End of interrupt: acknowledge the slave first, then the master. */
void pic_send_eoi(uint8_t irq);

#endif /* GENESIS_ARCH_I686_PIC_H */
