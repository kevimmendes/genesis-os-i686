# Project Genesis - build system.
#
# Target: 32-bit x86 (i686), the architecture of the reference machine - a
# Dell Latitude D600 with an Intel Pentium M. A 64-bit kernel cannot boot there,
# so nothing in this tree is built for x86_64.
#
# The kernel is freestanding: no hosted C library, no crt0, no dynamic linking.
# It builds with the host GCC (`-m32`) and binutils, which every Linux
# distribution ships, so no cross-compiler is required to get started.
#
# Override any tool on the command line, for example:
#   make CC=i686-elf-gcc LD=i686-elf-ld
#   make QEMU=qemu-system-x86_64

# GNU make predefines CC=cc and LD=ld, and ?= does not override a defined
# variable, so only take over those two when they still hold make's built-in
# default.
ifeq ($(origin CC),default)
CC := gcc
endif
ifeq ($(origin LD),default)
LD := ld
endif

OBJCOPY ?= objcopy
QEMU    ?= qemu-system-i386

BUILD    := build
KERNEL   := $(BUILD)/kernel.elf
ISO_NAME := genesis.iso

# -m32 builds 32-bit code with the host toolchain. -march=i686 matches the
# Pentium M (and everything newer); SSE/MMX are disabled so no FPU/vector state
# has to be saved anywhere.
CFLAGS := -m32 -march=i686 -std=gnu11 -O2 -g \
          -ffreestanding -fno-builtin -fno-stack-protector \
          -fno-pic -fno-pie -mno-mmx -mno-sse -mno-sse2 \
          -fno-asynchronous-unwind-tables -fno-unwind-tables \
          -Wall -Wextra -Werror -Iinclude

# Assembly only needs the mode and the include path; the C-only flags above
# would be rejected as unused arguments when assembling a .s file.
ASFLAGS := -m32 -g -Iinclude

# On Windows the default recipe shell is cmd.exe, which has no mkdir -p or rm.
ifeq ($(OS),Windows_NT)
SHELL := sh
endif

LDFLAGS := -m elf_i386 -nostdlib -static -z max-page-size=0x1000 \
           -T boot/linker.ld

# The AI agent link is a future option, not part of the default system: the
# professor's agent will plug into it later. Build with `make AGENT=1` to get
# the `ai` command and kernel/agent.c back.
AGENT ?= 0

# Source discovery is recursive up to three levels, so a subsystem can move into
# its own directory (kernel/arch/i686/..., kernel/mm/...) without touching this
# file. `find` is not used: make's wildcard keeps it shell-independent.
C_SOURCES    := $(filter-out kernel/agent.c,$(wildcard kernel/*.c kernel/*/*.c kernel/*/*/*.c))
BOOT_SOURCES := $(wildcard boot/*.s)
KERNEL_ASM   := $(wildcard kernel/*/*.s kernel/*/*/*.s)
HEADERS      := $(wildcard include/*.h include/*/*.h include/*/*/*.h)

ifeq ($(AGENT),1)
CFLAGS  += -DGENESIS_ENABLE_AGENT
C_SOURCES += kernel/agent.c
endif

OBJECTS := $(patsubst boot/%.s,$(BUILD)/boot/%.o,$(BOOT_SOURCES)) \
           $(patsubst kernel/%.c,$(BUILD)/kernel/%.o,$(C_SOURCES)) \
           $(patsubst kernel/%.s,$(BUILD)/kernel/%.o,$(KERNEL_ASM))

.PHONY: all kernel check run test iso emulate splash clean distclean help

all: kernel

kernel: $(KERNEL)

$(BUILD)/boot/%.o: boot/%.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/kernel/%.o: kernel/%.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel/%.o: kernel/%.s
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(KERNEL): $(OBJECTS) boot/linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "built $@"

# Verify the tools before blaming the code.
check:
	@sh scripts/check-toolchain.sh

# Boot the kernel in QEMU, with the VGA window on screen and the serial console
# on this terminal. Stop it with Ctrl-C.
#
# The 32-bit Multiboot1 image is loaded directly with -kernel, exactly the way
# GRUB loads it from the ISO, so no ISO and no GRUB tools are needed here.
run: $(KERNEL)
	$(QEMU) -kernel $(KERNEL) -m 128M -serial stdio

# Reproducible headless boot test: capture serial output and check the result.
test: $(KERNEL)
	@sh scripts/run-headless-test.sh $(KERNEL)

# Build a GRUB ISO. Requires grub-mkrescue and xorriso (a Linux/GRUB host).
# This is the artifact to burn to a CD-R for the reference laptop.
iso: $(KERNEL)
	@sh scripts/make-iso.sh $(KERNEL)

# Run the kernel in-process with Unicorn (Python) and read the VGA text buffer.
# Useful on machines without QEMU: it exercises the real entry path and the
# screen driver, but skips firmware and GRUB.
emulate: $(KERNEL)
	@python3 tests/emulate_boot.py $(KERNEL)
	@python3 tests/emulate_boot.py $(KERNEL) setup

# Rebuild the boot-splash C source from the 80x48 drawing in tools/.
# Only needed after editing tools/splash-ascii.txt; the generated file is
# checked in, so a normal build does not require Python.
splash:
	@python3 tools/gen-splash.py

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -rf iso $(ISO_NAME)

help:
	@echo "Project Genesis (i686)"
	@echo "  make            Build build/kernel.elf"
	@echo "  make check      Report tool versions and missing tools"
	@echo "  make run        Boot the kernel in QEMU (VGA window + serial)"
	@echo "  make test       Headless QEMU boot, checks the serial output"
	@echo "  make emulate    Boot in Unicorn, drive the shell, check the screen"
	@echo "  make AGENT=1    Build with the optional AI agent link (future work)"
	@echo "  make splash     Regenerate the boot splash from tools/splash-ascii.txt"
	@echo "  make iso        Build a GRUB-bootable ISO (needs grub-mkrescue)"
	@echo "  make clean      Remove build output"
