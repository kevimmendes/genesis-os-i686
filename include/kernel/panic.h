/*
 * Project Genesis - fatal error reporting.
 *
 * A panic never returns. It disables interrupts, prints why, and parks the CPU
 * so the message stays on screen - on the reference laptop the screen is the
 * only console, and a reset would hide the evidence.
 */

#ifndef GENESIS_KERNEL_PANIC_H
#define GENESIS_KERNEL_PANIC_H

struct interrupt_frame;

void panic(const char *message) __attribute__((noreturn));

/* Report an unhandled CPU exception with its vector, error code and the
 * interrupted instruction pointer, then halt. */
void exception_report(const char *name, const struct interrupt_frame *frame)
    __attribute__((noreturn));

#endif /* GENESIS_KERNEL_PANIC_H */
