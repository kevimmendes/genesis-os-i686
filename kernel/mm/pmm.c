/*
 * Project Genesis - physical frame allocator (bitmap based).
 *
 * The whole bitmap starts out as "in use", the region above the first MiB is
 * released, and then the kernel image is marked used again. That order means a
 * missing or short memory map can never hand out the frame the kernel is
 * running from.
 */

#include <mm/pmm.h>

#define MAX_FRAMES       (512u * 1024u * 1024u / PMM_FRAME_SIZE)   /* 512 MiB, 131072 frames */
#define BITMAP_WORDS     (MAX_FRAMES / 32u)
#define FIRST_MIB        0x100000u
#define MAX_ADDRESS      (MAX_FRAMES * PMM_FRAME_SIZE)

static uint32_t used_bitmap[BITMAP_WORDS];
static uint32_t total_frames;
static uint32_t free_frames;

static void frame_mark_used(uint32_t frame)
{
    used_bitmap[frame / 32u] |= (uint32_t)1u << (frame % 32u);
}

static void frame_mark_free(uint32_t frame)
{
    used_bitmap[frame / 32u] &= ~((uint32_t)1u << (frame % 32u));
}

static int frame_is_used(uint32_t frame)
{
    return (used_bitmap[frame / 32u] >> (frame % 32u)) & 1u;
}

void pmm_init(uint32_t memory_upper_kib, uint32_t kernel_end_address)
{
    uint32_t usable_end = FIRST_MIB + memory_upper_kib * 1024u;
    uint32_t reserved_frames;
    uint32_t frame;

    if (memory_upper_kib == 0) {
        /* No memory information: manage nothing rather than guess. */
        total_frames = 0;
        free_frames = 0;
        return;
    }

    if (usable_end > MAX_ADDRESS) {
        usable_end = MAX_ADDRESS;
    }

    total_frames = usable_end / PMM_FRAME_SIZE;
    if (total_frames > MAX_FRAMES) {
        total_frames = MAX_FRAMES;
    }

    for (uint32_t word = 0; word < BITMAP_WORDS; word++) {
        used_bitmap[word] = 0xFFFFFFFFu;
    }
    free_frames = 0;

    for (frame = FIRST_MIB / PMM_FRAME_SIZE; frame < total_frames; frame++) {
        frame_mark_free(frame);
        free_frames++;
    }

    /* Low memory (IVT, BIOS, VGA, EBDA), the kernel image and the bitmap. */
    reserved_frames = (kernel_end_address + PMM_FRAME_SIZE - 1u) / PMM_FRAME_SIZE;
    if (reserved_frames > total_frames) {
        reserved_frames = total_frames;
    }

    for (frame = 0; frame < reserved_frames; frame++) {
        if (frame_is_used(frame) == 0) {
            frame_mark_used(frame);
            free_frames--;
        }
    }
}

uint32_t pmm_alloc_frame(void)
{
    for (uint32_t word = 0; word < BITMAP_WORDS; word++) {
        uint32_t free_bits = ~used_bitmap[word];

        while (free_bits != 0) {
            uint32_t bit = (uint32_t)__builtin_ctz(free_bits);
            uint32_t frame = word * 32u + bit;

            if (frame >= total_frames) {
                return 0;
            }

            used_bitmap[word] |= (uint32_t)1u << bit;
            free_frames--;
            return frame * PMM_FRAME_SIZE;
        }
    }

    return 0;
}

void pmm_free_frame(uint32_t frame_address)
{
    uint32_t frame;

    if (frame_address == 0 || (frame_address % PMM_FRAME_SIZE) != 0) {
        return;
    }

    frame = frame_address / PMM_FRAME_SIZE;
    if (frame >= total_frames) {
        return;
    }

    if (frame_is_used(frame) != 0) {
        frame_mark_free(frame);
        free_frames++;
    }
}

uint32_t pmm_total_frames(void)
{
    return total_frames;
}

uint32_t pmm_free_frames(void)
{
    return free_frames;
}

uint32_t pmm_used_frames(void)
{
    return total_frames - free_frames;
}

uint32_t pmm_total_kib(void)
{
    return total_frames * (PMM_FRAME_SIZE / 1024u);
}
