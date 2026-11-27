# Project Genesis
**A small, complete 32-bit operating system you can read end to end.**

Project Genesis is an educational x86 (i686) operating system written in C and
Assembly. It boots through GRUB (or QEMU's Multiboot loader), brings up the CPU
tables, the interrupt controller, a periodic timer, a PS/2 keyboard, physical
memory management and an interactive shell — and it fits in a few thousand lines
you can actually follow.

> **Status:** M1–M6 done — boots, handles interrupts, manages memory, answers a shell
> **Version:** 0.3.0
> **Target:** i686 / 32-bit x86 (Pentium M class and newer)
> **Languages:** C and x86 Assembly
> **Boot protocol:** GRUB Multiboot1
> **Test environment:** QEMU, plus a no-QEMU Unicorn harness that drives the shell
> **Reference machine:** Dell Latitude D600 (see [docs/HARDWARE.md](docs/HARDWARE.md))

## Why 32-bit?

The machine this project is written for is a Dell Latitude D600: an Intel
Pentium M 1.40 GHz with 512 MB of RAM, an ATI Radeon 9000 and a 20 GB IDE disk.
That CPU is 32-bit only. A 64-bit kernel cannot execute on it at all, so the
whole tree targets i686 and runs in 32-bit protected mode with paging disabled.
A system this small does not need more.

## What works today

**Boot and output**
- A Multiboot1 image that GRUB loads (`multiboot /boot/kernel.elf`) and that QEMU also accepts with `-kernel`.
- A kernel-owned flat GDT, explicit `.bss` clearing and a 16 KiB kernel stack.
- A full-screen boot splash drawn from the ASCII art in `tools/splash-ascii.txt`, held for about 2.5 seconds.
- A VGA text console (80×25, scrolling, hardware cursor) — the only output that exists on the laptop.
- A COM1 serial console at 115200 8N1, used when a UART answers its probe.
- Multiboot information decoding: protocol, bootloader name and usable memory.
- CPUID vendor identification, so the machine can be confirmed from the kernel itself.

**Interrupts and devices**
- An IDT with 32 exception stubs and 16 IRQ stubs; unhandled exceptions print their vector, error code and instruction pointer instead of silently resetting.
- The 8259 PIC remapped to vectors 32–47, so hardware IRQs no longer collide with CPU exceptions.
- A 100 Hz timer (8254 channel 0) that counts ticks and drives `uptime`.
- A PS/2 keyboard driver (scan code set 1, Shift and Backspace) feeding a line editor.

**Memory**
- A bitmap physical frame allocator built from the loader's memory size; the first MiB and the kernel image are always reserved.
- A small kernel heap (`kmalloc`) that takes 4 KiB frames from the frame allocator.
- `mem` and `info` commands that report exactly what the allocator believes.

**Shell and small tools**
- `help`, `clear`, `info`, `mem`, `uptime`, `echo`, `setup`, `splash`, `reboot`, `halt`.
- `calc <a> <op> <b>` for integer arithmetic and `hexdump [addr] [n]` to inspect memory.
- See [docs/SHELL.md](docs/SHELL.md).

**Future option: AI agent link (off by default)**
- The `ai <question>` command and `kernel/agent.c` are a prepared socket for the
  agent the professor will build: the laptop sends the question over COM1 and a
  PC runs the model. It is **not** part of the default system.
- Build it with `make AGENT=1` to get the command back. The design, protocol and
  host bridge are documented in [docs/AGENT.md](docs/AGENT.md) and
  [host/README.md](host/README.md).

**Setup wizard**
- The ISO shows a GRUB menu for 15 seconds with a live session and an
  **Install Project Genesis** entry that asks for language, keyboard layout
  (US/PT/ES) and how much disk space to use, then confirms the choices.
- The same wizard opens from the live shell with the `setup` command.
- Nothing is written to the disk yet: that needs the ATA driver and a
  filesystem (M8). The wizard says so on screen instead of pretending. See
  [docs/INSTALL.md](docs/INSTALL.md).

**Networking and SSH** are not implemented yet, and are not a small step: they
need a PCI network driver, an IP/TCP stack, a cryptographic library and a
filesystem before an SSH server can exist. [docs/NETWORK.md](docs/NETWORK.md)
lays out the stages honestly, starting with the Broadcom 570x adapter the
reference laptop has.

## Building

Requires GCC with 32-bit support (`gcc-multilib` on Debian/Ubuntu), binutils, GNU Make.

```sh
make check     # report the toolchain (and whether -m32 works)
make           # build build/kernel.elf
make run       # boot it in QEMU: VGA window + serial on the terminal
make test      # headless QEMU boot, checks the serial output
make iso       # GRUB ISO for real hardware (needs grub-mkrescue, xorriso)
```

No QEMU on your machine? `make emulate` runs the kernel in-process with Unicorn
(`pip install unicorn`), types `help`, `mem`, `calc`, `hexdump` and `halt` at the
prompt through a fake 8042, then opens the setup wizard twice (from the ISO's
install entry and from the `setup` command) and checks the splash and the output
on the VGA buffer.

On boot you first see the splash screen (the art, no text), and after about 2.5
seconds the screen is cleared for the report and the shell:

```text
Project Genesis 0.3.0
===============
target     : i686 (32-bit x86)
boot magic : 0x2badb002
protocol   : multiboot1
bootloader : GRUB 2.06
memory     : 639 KiB below 1 MiB, 130048 KiB above 1 MiB (128 MiB usable)
cpu vendor : GenuineIntel
cpu mode   : 32-bit protected mode, paging off
console    : VGA text 80x25 (no serial UART detected)
interrupts : PIC at vectors 32-47, PIT 100 Hz, PS/2 keyboard, 0 handled
frames     : 32496 free of 32768 (131072 KiB managed, 4 KiB each)
heap       : heap allocator online
result     : OK - kernel entry reached, interrupts running

type 'help' and press Enter for the command list.
genesis> help
Commands:
  help           show this list
  ...
genesis>
```

The memory and frame numbers depend on how much RAM the loader reports.

## Repository layout

```text
Project-Genesis/
├── boot/           # Multiboot header, 32-bit entry, GDT, linker script, grub.cfg
├── docs/           # Architecture, hardware, install, agent, shell, roadmap, toolchain
├── include/        # Kernel headers mirroring kernel/ (arch, drivers, kernel, lib, mm)
├── kernel/
│   ├── main.c      # kmain: brings the subsystems up in order, then starts the shell
│   ├── setup.c     # installer wizard: language, keyboard, disk space, confirm
│   ├── agent.c     # AI agent link over COM1 (question out, answer in)
│   ├── shell.c     # line editor and built-in commands
│   ├── panic.c     # panic and unhandled-exception reports
│   ├── report.c    # the kernel's self description (banner, boot info, machine)
│   ├── arch/i686/  # IDT + stubs, PIC, PIT, VGA, serial, splash, CPUID, boot info
│   ├── drivers/    # PS/2 keyboard
│   ├── lib/        # console, string routines
│   ├── mm/         # physical frame allocator, kernel heap
│   ├── sched/      # empty until M7
│   └── fs/         # empty until M8
├── host/           # Python bridge that runs the model on a PC (agent link)
├── scripts/        # Toolchain check, ISO creation, headless boot test
├── tests/          # Test plan and the Unicorn harness
├── tools/          # Original splash art and the generator for its C source
├── .gitignore
├── CONTRIBUTING.md
├── LICENSE
├── Makefile
└── README.md
```

Every source file has a header comment explaining what it does and why; the
headers mirror the implementation paths (`include/mm/pmm.h` belongs to
`kernel/mm/pmm.c`).

## Development principles
1. **Small steps:** one understandable feature per change.
2. **Test in an emulator:** do not experiment on your main computer's boot disk.
3. **Document decisions:** record why an approach was chosen.
4. **Keep the kernel freestanding:** do not assume a hosted operating-system C library exists.
5. **Verify every milestone:** a feature is not complete until its test is documented.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how the pieces fit,
[docs/ROADMAP.md](docs/ROADMAP.md) for the milestone list and
[docs/TOOLCHAIN.md](docs/TOOLCHAIN.md) for the exact tool versions and commands.

## Safety
Use QEMU or another disposable virtual machine for experiments. Booting the ISO
on the reference laptop is safe as long as you boot from the CD and do not
install anything over your existing system.

## License
See [LICENSE](LICENSE).
