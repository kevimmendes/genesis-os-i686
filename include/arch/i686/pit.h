/*
 * Project Genesis - 8254 programmable interval timer (i686).
 *
 * Channel 0 drives IRQ0 at a fixed frequency and is the kernel's time base.
 * Channel 2 is left alone for the boot splash, which reads its output bit
 * before interrupts exist (see arch/i686/splash.c).
 */

#ifndef GENESIS_ARCH_I686_PIT_H
#define GENESIS_ARCH_I686_PIT_H

#include <stdint.h>

/* The 8254 input clock: 1.193182 MHz. */
#define PIT_INPUT_HZ 1193182u

/* Program channel 0 for `hz` interrupts per second, register the tick handler
 * and unmask IRQ0. Frequencies outside 19..1193181 Hz are clamped to 100. */
void pit_init(uint32_t hz);

/* Ticks since pit_init(), incremented from the interrupt handler. */
uint32_t timer_ticks(void);

/* Whole seconds since boot, derived from the tick count. */
uint32_t timer_seconds(void);

/* The frequency the timer was actually programmed with. */
uint32_t timer_hz(void);

#endif /* GENESIS_ARCH_I686_PIT_H */
