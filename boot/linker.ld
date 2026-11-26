/*
 * Project Genesis - linker script (32-bit i686).
 *
 * The kernel is linked at the traditional 1 MiB mark, where the Multiboot1
 * loader puts it, and runs with paging disabled: virtual addresses are physical
 * addresses, so no higher-half offset is needed. The boot header must be the
 * first bytes of the first loadable section.
 */

ENTRY(_start)

SECTIONS
{
    . = 1M;

    .boot : ALIGN(4K)
    {
        KEEP(*(.multiboot))
    }

    .text : ALIGN(4K)
    {
        *(.text.boot)
        *(.text .text.*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4K)
    {
        *(.data .data.*)
    }

    /* __bss_start/__bss_end let the boot path zero .bss explicitly. */
    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        __bss_end = .;
    }

    /* First byte after the image: the frame allocator reserves everything
       below it, so the kernel can never hand out its own memory. */
    kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame .eh_frame_hdr)
        *(.note .note.*)
    }
}
