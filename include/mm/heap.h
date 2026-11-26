/*
 * Project Genesis - kernel heap.
 *
 * Deliberately minimal: a bump allocator that asks the frame allocator for one
 * 4 KiB frame at a time. It exists so kernel code can allocate small objects
 * without a page table, and it is honest about what it does not do: memory is
 * never reused. A real allocator (free lists, paging back-end) is a later
 * milestone; the interface is the one that will survive.
 */

#ifndef GENESIS_MM_HEAP_H
#define GENESIS_MM_HEAP_H

#include <stdint.h>

/* Reset the allocator. Call once, after pmm_init(). */
void heap_init(void);

/* Allocate `size` bytes, 8-byte aligned. Returns 0 when out of memory or when
 * `size` exceeds one frame. */
void *kmalloc(uint32_t size);

/* Present for interface completeness: the bump allocator cannot reuse memory,
 * so this only exists to make call sites read normally. */
void kfree(void *pointer);

/* Bytes handed out since heap_init(). */
uint32_t heap_used(void);

#endif /* GENESIS_MM_HEAP_H */
