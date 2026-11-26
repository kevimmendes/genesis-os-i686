/*
 * Project Genesis - interrupt stubs (32-bit x86).
 *
 * The CPU can only jump to a flat address, so each vector gets a tiny stub
 * that normalises the stack and reaches one common routine:
 *
 *      [vector][error][eip][cs][eflags]   <- built here and by the CPU
 *
 * Exceptions that push an error code (8, 10-14, 17) already have it on the
 * stack; for the others the stub pushes a zero, so every frame looks the same
 * to the C dispatcher. IRQ stubs are identical to no-error exception stubs.
 */

.section .text
.code32

.macro ISR_NOERR number
.global isr\number
isr\number:
    cli
    pushl $0
    pushl $\number
    jmp isr_common
.endm

.macro ISR_ERR number
.global isr\number
isr\number:
    cli
    pushl $\number
    jmp isr_common
.endm

.macro IRQ_ENTRY number, vector
.global irq\number
irq\number:
    cli
    pushl $0
    pushl $\vector
    jmp isr_common
.endm

/* CPU exceptions, vectors 0-31. */
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

/* Hardware IRQs, remapped to vectors 32-47. */
IRQ_ENTRY 0, 32
IRQ_ENTRY 1, 33
IRQ_ENTRY 2, 34
IRQ_ENTRY 3, 35
IRQ_ENTRY 4, 36
IRQ_ENTRY 5, 37
IRQ_ENTRY 6, 38
IRQ_ENTRY 7, 39
IRQ_ENTRY 8, 40
IRQ_ENTRY 9, 41
IRQ_ENTRY 10, 42
IRQ_ENTRY 11, 43
IRQ_ENTRY 12, 44
IRQ_ENTRY 13, 45
IRQ_ENTRY 14, 46
IRQ_ENTRY 15, 47

/*
 * Save the general registers, point the segment registers at the flat kernel
 * data selector, hand the frame to C, then undo everything and return with
 * iret, which restores EIP, CS and EFLAGS (including the interrupt flag).
 */
isr_common:
    pusha

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl %esp
    call interrupt_dispatch
    addl $4, %esp

    popa
    addl $8, %esp
    iret

/*
 * Tables of stub addresses, so idt.c can install all gates in a loop instead
 * of naming 48 symbols.
 */
.section .rodata
.global isr_stub_table
isr_stub_table:
    .long isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7
    .long isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15
    .long isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    .long isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

.global irq_stub_table
irq_stub_table:
    .long irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7
    .long irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15

.section .note.GNU-stack, "", @progbits
