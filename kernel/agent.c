/*
 * Project Genesis - AI agent link over COM1.
 *
 * The kernel half of the protocol described in include/kernel/agent.h: write
 * one prompt line, stream the answer to the console until the host sends EOT.
 *
 * Two guards keep the shell alive:
 *   - the answer is bounded, so a noisy line cannot fill memory;
 *   - waiting is bounded by the timer, with a poll limit as fallback for an
 *     environment where timer interrupts are not delivered (the Unicorn test).
 */

#include <arch/i686/io.h>
#include <arch/i686/pit.h>
#include <arch/i686/serial.h>
#include <kernel/agent.h>
#include <lib/console.h>

/* How long the host may take to start answering. Models are slow; a minute is
 * generous but still finite. */
#define AGENT_TIMEOUT_MS 60000u

/* Fallback used only while the timer is not ticking: roughly 0.2 s of port
 * reads on real hardware, and a small instruction budget in an emulator. */
#define AGENT_POLL_LIMIT 200000u

/* Longer answers are truncated; the shell line buffer is smaller anyway. */
#define AGENT_ANSWER_LIMIT 1024u

#define AGENT_END_OF_TEXT 0x04

void agent_ask(const char *prompt)
{
    uint32_t timeout_ticks;
    uint32_t start_ticks;
    uint32_t polls = 0;
    uint32_t received = 0;
    int timed_out = 0;

    if (serial_present() == 0) {
        console_puts("agent: no serial link. COM1 was not detected.\n");
        console_puts("       See docs/AGENT.md for the cable and the host bridge.\n");
        return;
    }

    /* One line, so the host can parse it with a plain readline(). */
    serial_putc('A');
    serial_putc('I');
    serial_putc(' ');
    for (const char *cursor = prompt; *cursor != '\0'; cursor++) {
        serial_putc(*cursor);
    }
    serial_putc('\n');

    console_puts("agent: waiting for the host model over COM1...\n");

    timeout_ticks = (AGENT_TIMEOUT_MS * timer_hz()) / 1000u;
    if (timeout_ticks == 0) {
        timeout_ticks = 1;
    }
    start_ticks = timer_ticks();

    for (;;) {
        char c;

        if (serial_try_getc(&c)) {
            polls = 0;

            if ((uint8_t)c == AGENT_END_OF_TEXT) {
                break;
            }

            if (received >= AGENT_ANSWER_LIMIT) {
                console_puts("\nagent: answer truncated at 1024 bytes.\n");
                break;
            }

            if (c != '\r') {
                console_putc(c);
            }
            received++;
            continue;
        }

        if (timer_ticks() != start_ticks) {
            if (timer_ticks() - start_ticks >= timeout_ticks) {
                timed_out = 1;
                break;
            }
        } else if (++polls > AGENT_POLL_LIMIT) {
            timed_out = 1;
            break;
        }

        /* Interrupts are on; sleep until the timer or the UART wakes us. */
        cpu_idle();
    }

    if (timed_out != 0) {
        console_puts("\nagent: the host did not answer (timeout).\n");
        console_puts("       Is agent_bridge.py running on the other end?\n");
        return;
    }

    console_putc('\n');
}
