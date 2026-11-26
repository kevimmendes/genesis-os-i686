/*
 * Project Genesis - the AI agent link (serial).
 *
 * The reference laptop cannot run a language model: 512 MB of RAM and a
 * single-core Pentium M are far below what any useful model needs. So the
 * system does the honest thing instead: it is the terminal, and a host PC runs
 * the model.
 *
 * The link is the 16550 serial port, which every D600 has. The protocol is one
 * line in each direction, so it can be debugged with any serial terminal:
 *
 *   kernel -> host   "AI <prompt>\n"
 *   host -> kernel   "<answer>" then a single 0x04 byte (EOT)
 *
 * docs/AGENT.md describes the host-side bridge.
 */

#ifndef GENESIS_KERNEL_AGENT_H
#define GENESIS_KERNEL_AGENT_H

/* Send the prompt over COM1, print the answer as it arrives, and return.
 *
 * Returns without printing anything except a diagnostic when there is no
 * serial port, when the host does not answer in time, or when the answer is
 * longer than the link buffer. Never blocks indefinitely and never touches the
 * network. */
void agent_ask(const char *prompt);

#endif /* GENESIS_KERNEL_AGENT_H */
