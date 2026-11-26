/*
 * Project Genesis - panic and exception reporting.
 */

#include <arch/i686/idt.h>
#include <arch/i686/io.h>
#include <kernel/panic.h>
#include <lib/console.h>

void panic(const char *message)
{
    cpu_disable_interrupts();

    console_puts("\n*** KERNEL PANIC: ");
    console_puts(message);
    console_puts(" ***\n");
    console_puts("system halted. It is safe to power off.\n");

    for (;;) {
        cpu_halt();
    }
}

void exception_report(const char *name, const struct interrupt_frame *frame)
{
    cpu_disable_interrupts();

    console_puts("\n*** CPU EXCEPTION: ");
    console_puts(name);
    console_puts(" ***\n");
    console_puts("vector     : ");
    console_put_dec(frame->vector);
    console_puts("\nerror code : ");
    console_put_hex(frame->error);
    console_puts("\neip        : ");
    console_put_hex(frame->eip);
    console_puts("\ncs         : ");
    console_put_hex(frame->cs);
    console_puts("\neflags     : ");
    console_put_hex(frame->eflags);
    console_puts("\nsystem halted after an unhandled exception.\n");

    for (;;) {
        cpu_halt();
    }
}
