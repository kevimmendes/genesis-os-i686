# Toolchain notes

The kernel is freestanding 32-bit x86 code, so it needs a compiler that can emit
`-m32` code without a hosted C library and a linker that can produce a static
ELF32 image with a custom linker script.

## Required tools

| Tool | Purpose | Tested |
|---|---|---|
| `gcc` with 32-bit support | compile and assemble | GCC 13.3.0 (Ubuntu 24.04) |
| `ld` (binutils) | link `build/kernel.elf` | GNU ld 2.42 |
| `make` | build driver | GNU Make 4.3 |

On Debian/Ubuntu, a 32-bit capable compiler is `gcc-multilib`:

```sh
sudo apt install build-essential gcc-multilib
```

`make check` compiles a one-line probe with `-m32` and tells you whether it
works, instead of letting you discover it halfway through a kernel build.

A cross-compiler such as `i686-elf-gcc` works too and is arguably cleaner:

```sh
make CC=i686-elf-gcc LD=i686-elf-ld
```

## Optional tools

| Tool | Purpose | Tested |
|---|---|---|
| `qemu-system-i386` | `make run`, `make test` | QEMU 8.x |
| `grub-mkrescue`, `xorriso` | `make iso` (real hardware) | GRUB 2.12 |
| `python3` + `unicorn` | `make emulate` (no QEMU needed) | Python 3.12, Unicorn 2.1.4 |

## Why `-m32` and `-march=i686`?

The reference machine is a Dell Latitude D600 with an Intel Pentium M — a
32-bit CPU. `-m32` produces exactly the code that CPU executes; `-march=i686`
allows the instructions every Pentium-class chip has, and nothing newer. SSE and
MMX are disabled (`-mno-sse -mno-mmx`) because no code saves floating-point or
vector state, and a kernel should not accidentally start using it.

## Build flags, explained

```text
-ffreestanding -fno-builtin      no hosted C library, no builtin substitutions
-fno-stack-protector             no stack canary, there is nothing to call on failure
-fno-pic -fno-pie                the kernel is linked at a fixed address (1 MiB)
-fno-asynchronous-unwind-tables  no .eh_frame data, there is no unwinder
-mno-sse -mno-mmx                no FPU/vector state to preserve
-Wall -Wextra -Werror            warnings are errors
```

Link:

```sh
ld -m elf_i386 -nostdlib -static -z max-page-size=0x1000 -T boot/linker.ld
```

## Boot protocol

Multiboot1 is used on purpose:

- GRUB reads it with `multiboot /boot/kernel.elf`, which is how the ISO boots
  the kernel on the laptop.
- QEMU's built-in `-kernel` loader accepts 32-bit Multiboot1 images directly,
  so `make run` and `make test` need neither GRUB nor an ISO.
- It is a few dozen lines of assembly to support, with no extra structures.

## Verified commands

```sh
make check          # toolchain report
make                # builds build/kernel.elf (ELF32, Intel 80386)
make emulate        # boots it in Unicorn, checks the VGA text buffer
make test           # boots it in QEMU headless, checks the serial log
make iso            # genesis.iso for the laptop
```

The last two require QEMU and GRUB respectively; `make` and `make emulate`
always work with just GCC, binutils and Python.
