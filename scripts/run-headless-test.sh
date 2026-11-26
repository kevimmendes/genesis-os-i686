#!/bin/sh
# Project Genesis - headless boot test.
#
# Boots the kernel in QEMU without a display, captures the serial console to a
# log file, and fails unless the expected boot diagnostics appear. This is the
# automated form of the "Boot" and "Console" rows in tests/README.md.
#
# The 32-bit Multiboot1 image is passed to -kernel, the same way GRUB loads it
# from the ISO, so GRUB and xorriso are not needed for this test.
#
# Usage: sh scripts/run-headless-test.sh [build/kernel.elf] [timeout seconds]

set -u

KERNEL=${1:-build/kernel.elf}
TIMEOUT=${2:-6}
QEMU=${QEMU:-qemu-system-i386}
LOG=build/serial.log

if [ ! -f "$KERNEL" ]; then
    echo "error: kernel not found at $KERNEL (run 'make' first)" >&2
    exit 1
fi

if ! command -v "$QEMU" >/dev/null 2>&1; then
    echo "error: $QEMU not found; run 'make check' to see what is missing" >&2
    exit 1
fi

mkdir -p build
: > "$LOG"

# -kernel: load the ELF through QEMU's built-in Multiboot loader.
# -serial file: writes the guest console straight to the log.
# -display none: no window, so the test can run unattended.
# -no-reboot / -no-shutdown: a triple fault stops instead of restarting forever.
"$QEMU" -kernel "$KERNEL" -m 128M \
        -display none -no-reboot -no-shutdown \
        -serial "file:$LOG" >/dev/null 2>&1 &
qemu_pid=$!

# Give the guest time to boot, then stop the emulator.
sleep "$TIMEOUT"
kill "$qemu_pid" 2>/dev/null
wait "$qemu_pid" 2>/dev/null

echo "serial output ($LOG):"
echo "---------------------"
cat "$LOG"
echo "---------------------"

status=0
check() {
    if grep -q "$1" "$LOG"; then
        echo "PASS: found '$1'"
    else
        echo "FAIL: missing '$1'"
        status=1
    fi
}

check "Project Genesis"
check "protocol   : multiboot1"
check "32-bit protected mode"
check "interrupts : PIC at vectors 32-47"
check "frames     :"
check "result     : OK"

if [ "$status" -eq 0 ]; then
    echo "result: boot test passed"
else
    echo "result: boot test FAILED"
fi

exit "$status"
