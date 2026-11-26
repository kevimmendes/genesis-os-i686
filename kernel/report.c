/*
 * Project Genesis - the kernel's self description.
 */

#include <arch/i686/cpu.h>
#include <arch/i686/idt.h>
#include <arch/i686/pit.h>
#include <kernel/report.h>
#include <kernel/version.h>
#include <lib/console.h>
#include <mm/pmm.h>

void report_banner(void)
{
    console_puts("\nProject Genesis " GENESIS_VERSION "\n");
    console_puts("===============\n");
    console_puts("target     : i686 (32-bit x86)\n");
}

void report_boot_info(const struct boot_info *info)
{
    console_puts("boot magic : ");
    console_put_hex(info->magic);
    console_puts("\n");

    console_puts("protocol   : ");
    console_puts(info->protocol);
    console_puts("\n");

    if (info->loader != 0) {
        console_puts("bootloader : ");
        console_puts(info->loader);
        console_puts("\n");
    }

    if (info->has_memory != 0) {
        console_puts("memory     : ");
        console_put_dec(info->memory_lower_kib);
        console_puts(" KiB below 1 MiB, ");
        console_put_dec(info->memory_upper_kib);
        console_puts(" KiB above 1 MiB (");
        console_put_dec((info->memory_upper_kib + 1024u) / 1024u);
        console_puts(" MiB usable)\n");
    }
}

void report_machine(void)
{
    console_puts("cpu vendor : ");
    console_puts(cpu_vendor());
    console_puts("\ncpu mode   : 32-bit protected mode, paging off\n");

    console_puts("console    : VGA text 80x25");
    if (console_serial_present() != 0) {
        console_puts(" + serial COM1");
    } else {
        console_puts(" (no serial UART detected)");
    }
    console_puts("\n");

    console_puts("interrupts : PIC at vectors 32-47, PIT ");
    console_put_dec(timer_hz());
    console_puts(" Hz, PS/2 keyboard, ");
    console_put_dec(interrupts_handled());
    console_puts(" handled\n");

    console_puts("frames     : ");
    console_put_dec(pmm_free_frames());
    console_puts(" free of ");
    console_put_dec(pmm_total_frames());
    console_puts(" (");
    console_put_dec(pmm_total_kib());
    console_puts(" KiB managed, 4 KiB each)\n");
}
