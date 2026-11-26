/*
 * Project Genesis - early boot path (32-bit x86 / i686).
 *
 * The reference machine is a Dell Latitude D600: an Intel Pentium M, which is
 * a 32-bit CPU. A 64-bit kernel cannot run there at all, so the whole kernel is
 * built for i686 and stays in 32-bit protected mode with paging disabled.
 *
 * GRUB - or QEMU's -kernel loader - jumps to _start in 32-bit protected mode
 * with paging off. This file:
 *   1. carries a Multiboot1 header, understood by GRUB and by QEMU -kernel;
 *   2. zeroes .bss, because not every loader promises to do it;
 *   3. loads a kernel-owned flat GDT and reloads the segment registers;
 *   4. calls kmain(magic, boot_info) with the loader hand-off preserved.
 *
 * Nothing here depends on a hosted C library or on the loader's GDT.
 */

.set MB1_MAGIC, 0x1BADB002
.set MB1_FLAGS, 0x00000007          /* align modules, memory info, cmdline */

/* ------------------------------------------------------------------------- */
/* Boot header. It must sit in the first 8 KiB of the image, 4-byte aligned.  */
/* ------------------------------------------------------------------------- */

.section .multiboot, "a"
.p2align 2
mb_header:
    .long MB1_MAGIC
    .long MB1_FLAGS
    .long -(MB1_MAGIC + MB1_FLAGS)
mb_header_end:

/* ------------------------------------------------------------------------- */
/* 32-bit entry point                                                        */
/* ------------------------------------------------------------------------- */

.section .text.boot, "ax"
.code32
.global _start
.type _start, @function
_start:
    cli                                 /* no interrupts until the IDT exists */
    cld

    /*
     * The loader's stack is not part of our contract, and the kernel stack
     * lives in .bss, which is about to be zeroed: set ESP first, then zero.
     * EAX (magic) and EBX (info address) must survive `rep stosb`, which
     * clobbers EAX/ECX/EDI, so park the magic in ESI meanwhile.
     */
    movl %eax, %esi
    movl $stack_top, %esp
    xorl %ebp, %ebp

    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    movl %esi, %eax                     /* Multiboot boot magic          */

    lgdt gdt_ptr
    ljmp $0x08, $1f                     /* reload CS from our GDT */
1:
    /* Reload the segments from CX, not AX: EAX will be restored from ESI
       afterwards, but AX must never be clobbered while the magic is in it. */
    movw $0x10, %cx
    movw %cx, %ds
    movw %cx, %es
    movw %cx, %fs
    movw %cx, %gs
    movw %cx, %ss

    movl %esi, %eax                     /* Multiboot boot magic */

    /* kmain(uint32_t magic, uint32_t boot_info); arguments are pushed right
       to left, and the caller cleans the stack afterwards. */
    pushl %ebx
    pushl %eax
    call kmain
    addl $8, %esp

    /* kmain must not return; halt the CPU if it ever does. */
2:
    cli
    hlt
    jmp 2b

.size _start, . - _start

/* ------------------------------------------------------------------------- */
/* Flat GDT: ring 0 code and data covering the whole 4 GiB address space.     */
/* ------------------------------------------------------------------------- */

.section .rodata
.p2align 3
gdt:
    .quad 0x0000000000000000            /* 0x00: null descriptor          */
    .quad 0x00CF9A000000FFFF            /* 0x08: code, base 0, limit 4 GiB */
    .quad 0x00CF92000000FFFF            /* 0x10: data, base 0, limit 4 GiB */
gdt_end:

.p2align 2
gdt_ptr:
    .word gdt_end - gdt - 1
    .long gdt

/* ------------------------------------------------------------------------- */
/* Kernel stack (16 KiB is plenty for a kernel this small)                    */
/* ------------------------------------------------------------------------- */

.section .bss
.p2align 4
stack_bottom:
    .skip 16384
stack_top:

.section .note.GNU-stack, "", @progbits
