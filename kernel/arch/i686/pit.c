/*
 * Project Genesis - 8254 channel 0 as the system clock.
 *
 * The tick handler does one thing: increment a counter. Anything that needs
 * time reads that counter, so an interrupt never prints, allocates or blocks.
 */

#include <arch/i686/idt.h>
#include <arch/i686/io.h>
#include <arch/i686/pic.h>
#include <arch/i686/pit.h>

#define PIT_CHANNEL0 0x40u
#define PIT_COMMAND  0x43u

/* channel 0, lobyte/hibyte, mode 3 (square wave), binary. */
#define PIT_MODE0    0x36u

static volatile uint32_t tick_count;
static uint32_t tick_frequency = 100u;

static void pit_on_tick(struct interrupt_frame *frame)
{
    (void)frame;
    tick_count++;
}

void pit_init(uint32_t hz)
{
    uint32_t divisor;

    if (hz < 19u || hz > PIT_INPUT_HZ) {
        hz = 100u;
    }

    divisor = PIT_INPUT_HZ / hz;
    if (divisor == 0) {
        divisor = 1;
    } else if (divisor > 65536u) {
        divisor = 65536u;
    }

    tick_frequency = PIT_INPUT_HZ / divisor;
    tick_count = 0;

    irq_install(0, pit_on_tick);

    outb(PIT_COMMAND, PIT_MODE0);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));

    pic_unmask_irq(0);
}

uint32_t timer_ticks(void)
{
    return tick_count;
}

uint32_t timer_seconds(void)
{
    return tick_count / tick_frequency;
}

uint32_t timer_hz(void)
{
    return tick_frequency;
}
