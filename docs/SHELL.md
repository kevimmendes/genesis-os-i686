# Shell reference

The shell is the system's only user interface. It runs in ring 0 on the boot
thread, reads one line at a time from the PS/2 keyboard and executes a built-in
command. There is no scheduler and no separate user program yet, so `halt` and
`reboot` have the machine to themselves.

## Editing

| Key | Effect |
|---|---|
| printable keys | appended to the line and echoed |
| Shift | upper case letters and shifted symbols |
| Backspace | deletes the last character |
| Enter | runs the line |

Lines longer than 127 characters are truncated. Scan codes without an ASCII
meaning (function keys, arrows, key releases) are ignored.

## Commands

| Command | What it does |
|---|---|
| `help` | prints the command list |
| `clear` | clears the VGA screen (and a captured serial log) |
| `info` | repeats the boot report plus CPU, console, interrupts and frame counts |
| `mem` | frame allocator and kernel heap usage |
| `uptime` | seconds and timer ticks since `pit_init` |
| `ai <question>` | *optional* (`make AGENT=1`): asks the host model over COM1 |
| `calc <a> <op> <b>` | integer arithmetic: `+`, `-`, `*`, `/`, `%` (unsigned 32-bit) |
| `hexdump [addr] [n]` | hex and ASCII dump, default `0x100000` for 64 bytes, at most 256 |
| `echo <text>` | prints the rest of the line |
| `splash` | draws the boot art again for 1.5 s, then clears the screen |
| `reboot` | pulses the 8042 reset line |
| `halt` | disables interrupts and stops the CPU |

Unknown commands print the offending line and a pointer to `help`.

## Why these commands

Every command is a window into a subsystem that exists: `uptime` reads the PIT
tick counter, `mem` reads the frame bitmap and the heap counter, `info` reads
the Multiboot hand-off and CPUID, `splash` re-runs the screen driver, and
`reboot`/`halt` exercise the lowest-level machine control. There is deliberately
no command that pretends to do something the kernel cannot do.

`ai` is the agent link and is **off by default**: it is a prepared socket for the
professor's agent, enabled with `make AGENT=1`. The laptop sends the question
over the serial cable and `host/agent_bridge.py` runs the model on a PC; see
[AGENT.md](AGENT.md).

`calc` and `hexdump` are the small utilities: arithmetic without a floating
point unit, and a memory viewer whose default address is the Multiboot header,
which is always mapped and always starts with `02 b0 ad 1b`. Giving `hexdump`
an address that is not mapped raises a page/segmentation fault; the exception
report prints the address and halts, which is the honest behaviour until the
kernel has a recoverable fault handler.

## Adding a command

1. Write a `static void print_<name>(void)` function in `kernel/shell.c`.
2. Add a branch to `execute_line()`, matching the command word.
3. Add it to `print_help()` and to the table above.
4. Add a line to `tests/emulate_boot.py`'s `TYPED_LINES` if it is worth an
   automated check, and extend the expected strings.
