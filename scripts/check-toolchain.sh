#!/bin/sh
# Project Genesis - toolchain check.
#
# Prints the version of every tool the build uses and fails when a required one
# is missing. Optional tools (QEMU, GRUB ISO creation) are reported but not
# required to build the kernel.
#
# Usage: sh scripts/check-toolchain.sh

set -u

missing=0

# report <label> <command> [version arguments...]
report() {
    label=$1
    command=$2
    shift 2

    if command -v "$command" >/dev/null 2>&1; then
        printf '  %-14s ' "$label"
        "$command" "$@" 2>&1 | head -n 1
    else
        printf '  %-14s MISSING (%s)\n' "$label" "$command"
        missing=$((missing + 1))
    fi
}

# report_optional <label> <command> <hint>
report_optional() {
    label=$1
    command=$2
    hint=$3

    if command -v "$command" >/dev/null 2>&1; then
        printf '  %-14s %s\n' "$label" "$(command -v "$command")"
    else
        printf '  %-14s not installed - %s\n' "$label" "$hint"
    fi
}

echo "Project Genesis - toolchain (target: i686, 32-bit x86)"
echo "------------------------------------------------------"
echo "host: $(uname -s) $(uname -m)"
echo
echo "required:"
report "compiler" "${CC:-gcc}" --version
report "linker" "${LD:-ld}" --version
report "assembler" "${CC:-gcc}" --version
report "make" "${MAKE:-make}" --version

# A 32-bit build with the host GCC needs the 32-bit target support that ships
# with gcc/binutils on every common distribution; check it instead of guessing.
probe_dir=$(mktemp -d 2>/dev/null || echo build/check-tmp)
mkdir -p "$probe_dir"
printf 'int probe;\n' > "$probe_dir/probe.c"
if "${CC:-gcc}" -m32 -ffreestanding -c "$probe_dir/probe.c" -o "$probe_dir/probe.o" 2>/dev/null; then
    printf '  %-14s %s\n' "32-bit mode" "OK (-m32 works)"
else
    printf '  %-14s %s\n' "32-bit mode" "MISSING (install gcc-multilib)"
    missing=$((missing + 1))
fi
rm -rf "$probe_dir"

echo
echo "optional:"
report_optional "objcopy" "${OBJCOPY:-objcopy}" "needed only for flat binary output"
report_optional "emulator" "${QEMU:-qemu-system-i386}" "needed for 'make run' and 'make test'"
report_optional "python3" "python3" "needed for 'make emulate' (plus: pip install unicorn)"
report_optional "grub-mkrescue" "grub-mkrescue" "needed only for 'make iso'"
report_optional "xorriso" "xorriso" "needed only for 'make iso'"

echo
if [ "$missing" -ne 0 ]; then
    echo "result: $missing required tool(s) missing"
    exit 1
fi

echo "result: required toolchain present"
