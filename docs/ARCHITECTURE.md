# Architecture notes

## Current target

Project Genesis targets **i686 (32-bit x86)** and runs in 32-bit protected mode
with paging disabled. Virtual addresses are physical addresses, the kernel is
linked and loaded at 1 MiB, and the whole image is a single static ELF32.

This is a deliberate fit to the reference hardware (see
[HARDWARE.md](HARDWARE.md)): the Pentium M cannot execute 64-bit code, and a
system this small gains nothing from long mode or from paging. Both can be added
later without moving the kernel: paging is a Milestone 5 concern.

## Boot chain

```text
BIOS / QEMU firmware
   └─ GRUB (from ISO)  or  QEMU -kernel
        └─ Multiboot1 entry: boot/boot.s
             1. cli / cld, save EAX (magic) and EBX (info pointer)
             2. set ESP to the kernel stack in .bss
             3. zero .bss between __bss_start and __bss_end
             4. lgdt with the kernel's flat GDT, reload CS and the data segments
             5. call kmain(magic, info)
        └─ kernel/main.c: kmain
             splash_show / splash_hold_ms      (no interrupts yet)
             console_init                       VGA + optional COM1
             boot_info_parse, report_banner
             pmm_init, heap_init, kmalloc probe (memory first)
             idt_init, pic_remap, pit_init, keyboard_init
             sti                                interrupts on
             report_machine, shell_run
```

The order is a dependency chain, not a preference: the splash cannot use
interrupts, the frame allocator needs the loader's memory size, and every
interrupt handler assumes the console and the allocator already exist.

## Interrupt path

```text
device (PIT, keyboard)
   └─ 8259 PIC, remapped: master 0x20-0x27, slave 0x28-0x2F
        └─ CPU reads gate from the IDT
             └─ kernel/arch/i686/interrupts.s: isrN / irqN
                  1. cli
                  2. push error code (0 when the CPU did not push one)
                  3. push vector
                  4. isr_common: pusha, load flat data segments,
                     push the frame pointer, call interrupt_dispatch
        └─ kernel/arch/i686/idt.c: interrupt_dispatch
             vector < 32   -> exception_report() and halt
             32 <= v < 48  -> C handler table, then pic_send_eoi()
             iret restores EIP, CS and EFLAGS
```

`struct interrupt_frame` in `include/arch/i686/idt.h` mirrors that stack
exactly; changing one without the other breaks every handler, so the two files
point at each other.

Handlers stay tiny. The timer only increments a counter; the keyboard only
pushes a character into a ring buffer. Printing, allocation and waiting happen
on the boot thread, never inside an interrupt.

## Layers

1. **Boot interface:** Multiboot1 header, 32-bit entry, flat GDT, `.bss`
   clearing, kernel stack.
2. **Architecture:** `kernel/arch/i686/` holds port I/O, the IDT and stubs, the
   PIC, the PIT, the serial driver, the VGA driver, the splash, boot-information
   decoding and CPUID. None of it leaks into `main.c`.
3. **Console:** `kernel/lib/console.c` fans out to every output device. The rest
   of the kernel prints through it and never touches a device directly.
4. **Drivers:** `kernel/drivers/keyboard.c` translates PS/2 scan codes. It is
   arch-specific in practice, but it is not part of the CPU support code.
5. **Memory:** `kernel/mm/pmm.c` is a bitmap frame allocator over the memory
   the loader reported; `kernel/mm/heap.c` is a bump allocator that takes whole
   frames from it. Identity addressing, no paging.
6. **Interface:** `kernel/shell.c` reads lines, echoes them and runs a fixed
   table of commands. It is ring-0 code on the boot thread: there is no
   scheduler and no user mode yet.
7. **Later layers:** scheduler, block devices, filesystem, user mode and system
   calls — see [ROADMAP.md](ROADMAP.md).

## Memory map, as the kernel sees it

```text
0x00000000 - 0x000FFFFF   low memory: IVT, BIOS, VGA, EBDA   (reserved)
0x00100000 - kernel_end   kernel image: .boot .text .rodata .data .bss
kernel_end - usable_end   free frames, 4 KiB each, bitmap-allocated
```

The frame allocator starts with every bit set, releases everything above 1 MiB,
then re-reserves everything below `kernel_end`. The bitmap lives in `.bss`, so
it is inside the image and therefore covered by that reservation. A loader that
reports no memory leaves the allocator with zero frames instead of a guess.

## Design rules

- **Hardware behind small interfaces.** A driver exposes the few functions the
  kernel needs; `main.c` stays readable.
- **No hosted C library.** No libc, no `printf`. `kernel/lib/string.c` exists
  because the compiler itself may emit `memcpy`/`memset` calls.
- **Interrupts do the minimum.** Count a tick, queue a byte, acknowledge the
  PIC. Never block, print or allocate in a handler.
- **Output must never freeze the kernel.** On the laptop there is no serial
  port, so the UART is probed and every transmit timeout is bounded. A missing
  or stuck device loses bytes; it cannot hang the machine.
- **Two outputs, one text.** Numbers are formatted once in the console layer, so
  the screen and the serial log always agree.
- **Everything documented with the code that implements it.**
