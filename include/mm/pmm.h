/*
 * Project Genesis - physical memory manager.
 *
 * One bit per 4 KiB frame, in a static bitmap. Frames are identity-addressed
 * because paging is off: a frame returned by pmm_alloc_frame() can be used as a
 * normal pointer. The first MiB and the kernel image are always reserved.
 *
 * The bitmap covers at most 512 MiB, which is far more than the reference
 * machine's 512 MB minus the reserved hole, and costs 16 KiB of .bss.
 */

#ifndef GENESIS_MM_PMM_H
#define GENESIS_MM_PMM_H

#include <stdint.h>

#define PMM_FRAME_SIZE 4096u

/* Reserve the low memory and the kernel image, then free everything above it.
 * `kernel_end_address` is the first byte after the kernel image. */
void pmm_init(uint32_t memory_upper_kib, uint32_t kernel_end_address);

/* Allocate one frame. Returns its physical address, or 0 when memory is full. */
uint32_t pmm_alloc_frame(void);

/* Give a frame back. Unaligned, out-of-range or already-free addresses are
 * ignored instead of corrupting the bitmap. */
void pmm_free_frame(uint32_t frame_address);

uint32_t pmm_total_frames(void);
uint32_t pmm_free_frames(void);
uint32_t pmm_used_frames(void);

/* Total managed memory in KiB (frames x 4 KiB). */
uint32_t pmm_total_kib(void);

#endif /* GENESIS_MM_PMM_H */
