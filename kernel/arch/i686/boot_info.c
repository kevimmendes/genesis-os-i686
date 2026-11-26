/*
 * Project Genesis - Multiboot1 information decoding.
 *
 * The loader hands the kernel a pointer to a flags-gated structure. It is read
 * defensively: only fields whose flag bit is set are ever touched, because
 * anything unset may contain garbage on real firmware.
 */

#include <arch/i686/boot_info.h>

/* Multiboot1 information structure (only the leading fields we read). */
struct multiboot1_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
} __attribute__((packed));

#define MB1_FLAG_MEMORY     (1u << 0)
#define MB1_FLAG_CMDLINE    (1u << 2)
#define MB1_FLAG_BOOTLOADER (1u << 9)

void boot_info_parse(uint32_t magic, uint32_t info_address, struct boot_info *out)
{
    out->magic = magic;
    out->protocol = "unknown";
    out->loader = 0;
    out->info_address = info_address;
    out->memory_lower_kib = 0;
    out->memory_upper_kib = 0;
    out->has_memory = 0;
    out->cmdline = 0;

    if (magic != MULTIBOOT1_BOOT_MAGIC || info_address == 0) {
        return;
    }

    out->protocol = "multiboot1";

    const struct multiboot1_info *info =
        (const struct multiboot1_info *)(uintptr_t)info_address;

    if ((info->flags & MB1_FLAG_BOOTLOADER) != 0 && info->boot_loader_name != 0) {
        out->loader = (const char *)(uintptr_t)info->boot_loader_name;
    }

    if ((info->flags & MB1_FLAG_CMDLINE) != 0 && info->cmdline != 0) {
        out->cmdline = (const char *)(uintptr_t)info->cmdline;
    }

    if ((info->flags & MB1_FLAG_MEMORY) != 0) {
        out->memory_lower_kib = info->mem_lower;
        out->memory_upper_kib = info->mem_upper;
        out->has_memory = 1;
    }
}
