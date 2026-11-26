/*
 * Project Genesis - bump allocator on top of the frame allocator.
 *
 * Each call that does not fit in the current frame takes one more 4 KiB frame
 * from pmm_alloc_frame() and bumps through it. The tail of a frame is wasted
 * when the next allocation does not fit; on a kernel this small that trade is
 * deliberate, and it keeps the code short enough to audit.
 */

#include <mm/heap.h>
#include <mm/pmm.h>

static uint8_t *bump;
static uint32_t remaining;
static uint32_t used_bytes;

void heap_init(void)
{
    bump = 0;
    remaining = 0;
    used_bytes = 0;
}

void *kmalloc(uint32_t size)
{
    void *result;

    if (size == 0 || size > PMM_FRAME_SIZE) {
        return 0;
    }

    size = (size + 7u) & ~7u;

    if (size > remaining) {
        uint32_t frame = pmm_alloc_frame();

        if (frame == 0) {
            return 0;
        }

        bump = (uint8_t *)(uintptr_t)frame;
        remaining = PMM_FRAME_SIZE;
    }

    result = bump;
    bump += size;
    remaining -= size;
    used_bytes += size;

    return result;
}

void kfree(void *pointer)
{
    (void)pointer;
}

uint32_t heap_used(void)
{
    return used_bytes;
}
