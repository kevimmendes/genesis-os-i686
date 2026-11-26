# Development roadmap

This roadmap is intentionally incremental. Complete and test each milestone before moving on.

## M0 — Toolchain and environment
- [x] Identify host OS and install build dependencies.
- [x] Pick the target architecture: i686, because the reference machine is 32-bit.
- [x] Use the host GCC with `-m32` (no cross-compiler needed to start).
- [x] Verify assembler, linker, GRUB utilities, and QEMU.
- [x] Document exact tool versions and commands (`make check`).

**Done when:** the required tools are available and a reproducible toolchain check is documented.

## M1 — First boot
- [x] Choose and document the boot protocol (Multiboot1, GRUB and QEMU `-kernel`).
- [x] Add the boot configuration (`boot/boot.s`, `boot/linker.ld`, `boot/grub.cfg`).
- [x] Add the kernel entry point and a kernel-owned flat GDT.
- [x] Link a minimal freestanding kernel.
- [ ] Create a bootable ISO *(script exists; needs a Linux host with grub-mkrescue)*.
- [x] Boot the image in QEMU / Unicorn.

**Done when:** the loader loads the kernel and it reaches its entry point reliably.

## M2 — Debug output
- [x] Initialize serial output, with a presence probe and bounded writes.
- [x] Add VGA text output, since the reference laptop has no serial port.
- [x] Add a console layer with decimal and hex formatting.
- [x] Report boot milestones, memory, CPU vendor and errors.

**Done when:** boot messages can be read from the screen and from the serial console.

## M3 — CPU descriptor tables
- [x] Define and load a Global Descriptor Table.
- [x] Define an Interrupt Descriptor Table (IDT).
- [x] Add exception stubs and readable exception reports.
- [ ] Prove the exception path with a deliberately triggered fault on real hardware.

**Done when:** a deliberately triggered test exception is reported in a controlled way.

## M4 — Interrupt handling
- [x] Define interrupt-frame structures.
- [x] Configure the platform interrupt controller (8259 PIC, or APIC later).
- [x] Add a periodic timer interrupt.
- [x] Track interrupt counts.
- [ ] Confirm IRQ delivery on the reference laptop (QEMU test covers emulation).

**Done when:** timer interrupts arrive and are handled without destabilizing the kernel.

## M5 — Memory management
- [ ] Read and document the bootloader memory map (Multiboot1 mmap).
- [x] Implement a physical page-frame allocator.
- [ ] Establish and document virtual-memory mappings (paging).
- [x] Add a kernel heap allocator.
- [ ] Add allocator stress tests (alloc/free loops, invalid addresses).

**Done when:** allocations and frees pass repeatable tests and invalid states are detected.

## M6 — Basic devices
- [x] Add a timer abstraction.
- [x] Add keyboard input (PS/2, as on the laptop).
- [ ] Define a consistent driver interface.

**Done when:** input events can be read and logged through the kernel.

## M7 — Scheduling
- [ ] Define thread/process state.
- [ ] Implement cooperative kernel threads first.
- [ ] Add a scheduler test.
- [ ] Consider preemption only after timer handling is stable.

**Done when:** two test threads can take turns and preserve their state.

## M8 — Storage and filesystem
- [x] Setup wizard that collects language, keyboard layout and disk space.
- [ ] Define block-device interfaces.
- [ ] Read from an emulated disk, then from the laptop's IDE disk (ATA PIO).
- [ ] Read and write a partition table.
- [ ] Choose a simple filesystem format for learning.
- [ ] Add read-only support before write support.
- [ ] Copy the kernel to disk and install GRUB (real installation).

**Done when:** a test file can be read from a disk image, and the setup wizard
can actually install the system instead of only recording the choices.

## M9 — User mode and system calls
- [ ] Define process address-space boundaries.
- [ ] Establish user-mode entry and return.
- [ ] Define a minimal system-call ABI.
- [ ] Validate pointers and arguments at the boundary.

**Done when:** a minimal user program can request a kernel service safely.

## M10 — Shell
- [x] Parse input into commands and arguments.
- [x] Add built-in commands such as `help`, `clear`, and `uptime`.
- [x] Add useful error messages.
- [x] Add small tools: `calc` and `hexdump`.
- [ ] Run the shell as a user-mode process (needs M9).

**Done when:** a user can interact with the kernel through a basic shell.

## M11 — AI agent link (future option, the professor's work)
See [AGENT.md](AGENT.md). **Off by default**: build with `make AGENT=1`.
- [x] Protocol and kernel side: `ai <question>` over COM1, EOT-terminated answers.
- [x] Host side: `host/agent_bridge.py` with Ollama and OpenAI-compatible backends.
- [x] Bounded waits, bounded answers, and a clear diagnostic when the link is missing.
- [ ] The agent itself: memory, tools, approval — decided and built by the professor.
- [ ] Test the link on the reference laptop with a null-modem cable.

**Done when:** the professor's agent answers through the D600 over a real cable.

## M12 — Networking
See [NETWORK.md](NETWORK.md) for the staged plan.
- [ ] Enumerate the PCI bus and a `lspci` command.
- [ ] Driver for the Broadcom 570x (BCM5705M) adapter.
- [ ] Ethernet, ARP, IPv4 and ICMP.
- [ ] UDP with a simple DHCP client (static address as fallback).
- [ ] TCP with retransmission and connection close.

**Done when:** the machine can answer a ping and exchange a TCP stream on the
reference hardware.

## M13 — SSH
- [ ] RNG with real entropy, SHA-256, ChaCha20-Poly1305 and Curve25519.
- [ ] SSH transport: version exchange, key exchange, encrypted records.
- [ ] User authentication (public key and password).
- [ ] Session channel with `exec` and `pty-req`.
- [ ] Host keys and utilities on a filesystem (needs M8).

**Done when:** an ordinary `ssh` client can log in and run a command on the
reference laptop.
