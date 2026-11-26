/*
 * Project Genesis - kernel entry point.
 *
 * Reached from boot/boot.s in 32-bit protected mode with paging disabled.
 *
 * The boot order matters:
 *   1. splash and reports need nothing but the screen;
 *   2. the frame allocator needs the memory size the loader reported;
 *   3. interrupts come last, because every handler assumes the console, the
 *      allocator and the interrupt controller are already set up.
 *
 * From the moment shell_run() is called the kernel is interactive: it answers
 * commands until the machine is reset or halted.
 */

#include <arch/i686/boot_info.h>
#include <arch/i686/idt.h>
#include <arch/i686/io.h>
#include <arch/i686/pic.h>
#include <arch/i686/pit.h>
#include <arch/i686/splash.h>
#include <drivers/keyboard.h>
#include <kernel/panic.h>
#include <kernel/report.h>
#include <kernel/setup.h>
#include <kernel/shell.h>
#include <lib/console.h>
#include <lib/string.h>
#include <mm/heap.h>
#include <mm/pmm.h>

/* First byte after the kernel image, defined by boot/linker.ld. */
extern uint8_t kernel_end;

void kmain(uint32_t magic, uint32_t info_address)
{
    struct boot_info info;
    char *heap_probe;

    /* The splash is drawn before any interrupt exists. */
    splash_show();
    splash_hold_ms(2500);

    console_init();

    report_banner();

    boot_info_parse(magic, info_address, &info);
    report_boot_info(&info);

    if (info.magic != MULTIBOOT1_BOOT_MAGIC) {
        console_puts("result     : FAILED - the boot hand-off is not Multiboot1\n");
        panic("boot hand-off is not Multiboot1");
    }

    pmm_init(info.has_memory != 0 ? info.memory_upper_kib : 0,
             (uint32_t)(uintptr_t)&kernel_end);
    heap_init();

    heap_probe = (char *)kmalloc(32);
    if (heap_probe != 0) {
        strcpy(heap_probe, "heap allocator online");
    }

    idt_init();
    pic_remap(0x20, 0x28);
    pit_init(100);
    keyboard_init();
    cpu_enable_interrupts();

    report_machine();

    console_puts("heap       : ");
    console_puts(heap_probe != 0 ? heap_probe : "unavailable");
    console_puts("\n");

    console_puts("result     : OK - kernel entry reached, interrupts running\n");

    /* The ISO's "Install" entry passes `setup` on the kernel command line. */
    if (setup_requested(info.cmdline) != 0) {
        setup_run();
    }

    shell_run(&info);
}
