#!/usr/bin/env python3
"""Project Genesis - boot the kernel in-process and read the screen.

This harness needs neither QEMU nor GRUB. It maps the kernel's ELF segments at
their load addresses, fakes the Multiboot1 hand-off (magic in EAX, information
pointer in EBX) and runs the real _start entry point.

It also stands in for the hardware the kernel expects: port 0x64 and port 0x60
are served from a queue of keyboard scan codes, so the shell can be typed into,
and the VGA text buffer is decoded at three moments (after the splash, at the
first prompt, when the shell halts).

What it does verify: the entry path, the flat GDT, stack setup, .bss zeroing,
the console, the screen driver, boot-information decoding, the frame allocator,
the heap, the keyboard driver, the shell and its commands.

What it does not verify: firmware, GRUB, real IRQ delivery (Unicorn emulates
neither the PIC nor the PIT), real hardware behaviour, or timing. Use
`make test` for the QEMU boot and the real machine for the rest.

Usage: python3 tests/emulate_boot.py [build/kernel.elf]
Requires: pip install unicorn
"""

import struct
import sys

try:
    from unicorn import (
        Uc,
        UC_ARCH_X86,
        UC_MODE_32,
        UC_HOOK_CODE,
        UC_HOOK_INSN,
        UC_HOOK_MEM_WRITE,
    )
    from unicorn.x86_const import (
        UC_X86_INS_IN,
        UC_X86_INS_OUT,
        UC_X86_REG_EAX,
        UC_X86_REG_EBX,
        UC_X86_REG_EIP,
        UC_X86_REG_ESP,
    )
except ImportError:  # pragma: no cover - environment problem, not kernel problem
    sys.exit("error: the 'unicorn' module is missing (pip install unicorn)")

MEMORY_BASE = 0x00000000
MEMORY_SIZE = 0x00400000            # 4 MiB covers real-ish low memory
VGA_BASE = 0xB8000
VGA_COLUMNS = 80
VGA_ROWS = 25

MULTIBOOT1_MAGIC = 0x2BADB002
MULTIBOOT_INFO_ADDRESS = 0x10000
BOOTLOADER_NAME_ADDRESS = 0x10200
BOOTLOADER_NAME = b"GRUB 2.06 (emulated)"
MEM_LOWER_KIB = 639
MEM_UPPER_KIB = 130048             # ~127 MiB, the QEMU -m 128M figure

MAX_INSTRUCTIONS = 25_000_000     # includes the splash wait, which times out here

# Typed into the shell once it starts. Scan code set 1, one byte per event, and
# 0x1C is Enter. Nothing consumes these before the shell: keyboard_init does not
# flush the controller (see kernel/drivers/keyboard.c).
SCANCODE_SET_1 = {
    "h": 0x23, "e": 0x12, "l": 0x26, "p": 0x19, "m": 0x32, "a": 0x1E,
    "t": 0x14, "c": 0x2E, "x": 0x2D, "d": 0x20, "u": 0x16,
    "i": 0x17, "o": 0x18, "s": 0x1F,
    "0": 0x0B, "1": 0x02, "2": 0x03, "3": 0x04, "4": 0x05,
    "5": 0x06, "6": 0x07, "7": 0x08, "8": 0x09, "9": 0x0A,
    "/": 0x35, " ": 0x39, "\n": 0x1C,
}

# Live session: type a few commands, then stop.
LIVE_LINES = "help\nmem\ncalc 81 / 9\nhexdump\nhalt\n"

# Installer: language 1 (Portuguese), keyboard 2 (PT), 10 GB, confirm, halt.
SETUP_LINES = "1\n2\n10\ns\nhalt\n"


def load_elf(path):
    """Load PT_LOAD segments at their physical address; return the entry point."""
    with open(path, "rb") as handle:
        image = handle.read()

    if image[:4] != b"\x7fELF":
        raise ValueError(f"{path} is not an ELF file")
    if image[4] != 1:
        raise ValueError("not a 32-bit ELF file; the kernel must be built for i686")

    # ELF32 header: e_entry, e_phoff, e_shoff, e_flags, e_ehsize, e_phentsize,
    # e_phnum, ... starting at offset 24.
    (entry, phoff, _shoff, _flags, _ehsize, phentsize, phnum) = struct.unpack_from(
        "<IIIIHHH", image, 24
    )

    segments = []
    for index in range(phnum):
        offset = phoff + index * phentsize
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz = struct.unpack_from(
            "<IIIIII", image, offset
        )
        if p_type == 1:  # PT_LOAD
            segments.append((p_paddr, image[p_offset:p_offset + p_filesz], p_memsz))

    return entry, segments


def write_memory(uc, address, data):
    uc.mem_write(address, data)


def build_multiboot_info(uc, cmdline=None):
    """Fake a Multiboot1 information structure: memory, loader name, cmdline."""
    cmdline_address = 0x10300

    if cmdline is not None:
        write_memory(uc, cmdline_address, cmdline.encode() + b"\x00")

    flags = 0x0201                      # memory info + bootloader name
    if cmdline is not None:
        flags |= 0x0004                 # cmdline present

    info = struct.pack(
        "<17I",
        flags,
        MEM_LOWER_KIB,
        MEM_UPPER_KIB,
        0,                                  # boot_device
        cmdline_address if cmdline else 0,  # cmdline
        0,                                  # mods_count
        0,                                  # mods_addr
        0, 0, 0, 0,                         # syms[0..3]
        0,                                  # mmap_length
        0,                                  # mmap_addr
        0, 0, 0,                            # drives_length, drives_addr, config_table
        BOOTLOADER_NAME_ADDRESS,
    )

    write_memory(uc, MULTIBOOT_INFO_ADDRESS, info)
    write_memory(uc, BOOTLOADER_NAME_ADDRESS, BOOTLOADER_NAME + b"\x00")


def typed_scancodes(lines):
    return bytearray(SCANCODE_SET_1[character] for character in lines)


def read_vga_text(uc):
    """Decode the 80x25 text buffer into a list of stripped lines."""
    raw = uc.mem_read(VGA_BASE, VGA_COLUMNS * VGA_ROWS * 2)
    lines = []
    for row in range(VGA_ROWS):
        cells = raw[row * VGA_COLUMNS * 2:(row + 1) * VGA_COLUMNS * 2]
        text = "".join(
            chr(cells[i]) if 32 <= cells[i] < 127 else " "
            for i in range(0, len(cells), 2)
        )
        lines.append(text.rstrip())
    return lines


def main():
    kernel = sys.argv[1] if len(sys.argv) > 1 else "build/kernel.elf"
    mode = sys.argv[2] if len(sys.argv) > 2 else "live"
    setup_mode = mode == "setup"
    entry, segments = load_elf(kernel)

    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(MEMORY_BASE, MEMORY_SIZE)

    for address, data, memsz in segments:
        uc.mem_write(address, data)
        if memsz > len(data):
            uc.mem_write(address + len(data), b"\x00" * (memsz - len(data)))

    build_multiboot_info(uc, "setup" if setup_mode else None)

    # The harness has no 8042 and no interrupt controller, so port 0x64 and port
    # 0x60 are served from a queue of scan codes. Reading the status port first
    # is what the driver does before taking a byte, so the two stay in step.
    keyboard = {
        "queue": typed_scancodes(SETUP_LINES if setup_mode else LIVE_LINES),
        "presented": False,
    }

    # A 16550 on COM1, so serial_init's scratch-register probe succeeds and the
    # second console is exercised without hardware.
    uart = {"scratch": 0, "rx": bytearray(), "tx": bytearray()}

    def io_hook(uc, port, size, user_data):
        if size != 1:
            return 0
        if port == 0x64:
            keyboard["presented"] = len(keyboard["queue"]) > 0
            return 0x01 if keyboard["presented"] else 0x00
        if port == 0x60:
            if keyboard["presented"] and keyboard["queue"]:
                keyboard["presented"] = False
                return keyboard["queue"].pop(0)
            keyboard["presented"] = False
            return 0
        if port == 0x3F8 + 7:                      # scratch register
            return uart["scratch"]
        if port == 0x3F8 + 5:                      # line status
            ready = 0x01 if uart["rx"] else 0x00
            return 0x20 | ready                    # THR empty, maybe data ready
        if port == 0x3F8:                          # received data
            return uart["rx"].pop(0) if uart["rx"] else 0
        return 0

    def io_out_hook(uc, port, size, value, user_data):
        if size != 1:
            return
        if port == 0x3F8 + 7:
            uart["scratch"] = value & 0xFF
        elif port == 0x3F8:
            uart["tx"].append(value & 0xFF)

    uc.hook_add(UC_HOOK_INSN, io_hook, None, 1, 0, UC_X86_INS_IN)
    uc.hook_add(UC_HOOK_INSN, io_out_hook, None, 1, 0, UC_X86_INS_OUT)

    stops = []
    uc.hook_add(UC_HOOK_CODE, lambda uc, address, size, data: stops.append(address))

    # The splash paints all 2000 cells of the text buffer before the console
    # clears it, so snapshot the screen the moment the last cell is written.
    splash = {"writes": 0, "lines": None}
    boot = {"lines": None}
    help = {"lines": None}
    summary = {"lines": None}
    wizard = {"markers": set(), "snapshots": []}
    vga_end = VGA_BASE + VGA_COLUMNS * VGA_ROWS * 2

    def capture_writes(uc, access, address, size, value, user_data):
        if VGA_BASE <= address < vga_end:
            splash["writes"] += 1
            if splash["lines"] is None and splash["writes"] >= VGA_COLUMNS * VGA_ROWS:
                splash["lines"] = read_vga_text(uc)
                return
            if splash["lines"] is None:
                return

            text = read_vga_text(uc)

            # Keep each answer before later output scrolls it away.
            if boot["lines"] is None and any("genesis>" in row for row in text):
                boot["lines"] = text
            if help["lines"] is None and "Commands:" in "\n".join(text):
                help["lines"] = text
            joined = "\n".join(text)
            if summary["lines"] is None and "Resumo da instalacao" in joined:
                summary["lines"] = text

            # The wizard clears the screen between steps, so keep each one.
            for marker in (
                "Idioma / Language / Idioma",
                "Teclado:",
                "Espaco para o sistema em GB",
            ):
                if marker not in wizard["markers"] and marker in joined:
                    wizard["markers"].add(marker)
                    wizard["snapshots"].append(text)

            if "system halted" in "\n".join(text):
                # The `halt` command ends the shell; stop before the CPU spends
                # the whole instruction budget in its halt loop.
                uc.emu_stop()

    uc.hook_add(UC_HOOK_MEM_WRITE, capture_writes)

    uc.reg_write(UC_X86_REG_EAX, MULTIBOOT1_MAGIC)
    uc.reg_write(UC_X86_REG_EBX, MULTIBOOT_INFO_ADDRESS)
    uc.reg_write(UC_X86_REG_ESP, 0x8000)
    uc.reg_write(UC_X86_REG_EIP, entry)

    try:
        uc.emu_start(entry, 0, count=MAX_INSTRUCTIONS)
    except Exception as error:  # unicorn reports a fault as an exception
        print(f"emulation stopped: {error}")
    finally:
        halted_at = uc.reg_read(UC_X86_REG_EIP)
        instruction_count = len(stops)

    lines = read_vga_text(uc)
    screen = "\n".join(lines)

    # Everything scrolls off as the shell prints; search every snapshot.
    for snapshot in (boot["lines"], help["lines"], summary["lines"]):
        if snapshot is not None:
            screen = screen + "\n" + "\n".join(snapshot)
    for snapshot in wizard["snapshots"]:
        screen = screen + "\n" + "\n".join(snapshot)

    print("VGA text buffer:")
    print("----------------")
    print("\n".join(line for line in lines if line))
    print("----------------")
    print(f"entry point      : 0x{entry:08x}")
    print(f"instructions run : {instruction_count}")
    print(f"stopped at       : 0x{halted_at:08x}")

    art = splash["lines"]
    if art is not None:
        print("splash screen at boot:")
        print("----------------")
        print("\n".join(art))
        print("----------------")

    if setup_mode:
        expected = [
            "Project Genesis - setup",
            "Idioma / Language / Idioma",
            "Teclado:",
            "Espaco para o sistema em GB",
            "Resumo da instalacao",
            "Preferencias guardadas para esta sessao",
            "nada foi escrito no disco",
            "system halted",
        ]
    else:
        expected = [
            "Project Genesis",
            "protocol   : multiboot1",
            "32-bit protected mode",
            "interrupts : PIC at vectors 32-47",
            "result     : OK",
            "genesis>",
            "Commands:",
            "frames     :",
            "81 / 9 = 9",
            "0x00100000:",
            "system halted",
        ]

    status = 0
    for needle in expected:
        if needle in screen:
            print(f"PASS: found '{needle}'")
        else:
            print(f"FAIL: missing '{needle}'")
            status = 1

    print("mode             :", "setup wizard" if setup_mode else "live session")

    art_rows = [line for line in (art or []) if line]
    art_ink = sum(len(line.replace(" ", "")) for line in art_rows)
    if len(art_rows) < 24 or art_ink < 1500:
        print("FAIL: the splash art was not painted over the screen")
        status = 1
    else:
        print("PASS: splash art painted over the whole screen")

    if instruction_count >= MAX_INSTRUCTIONS:
        print("FAIL: instruction limit reached (the kernel never halted)")
        status = 1

    print("result: emulate test passed" if status == 0 else "result: emulate test FAILED")
    return status


if __name__ == "__main__":
    sys.exit(main())
