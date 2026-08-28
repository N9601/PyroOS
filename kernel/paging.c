/* ============================================================================
 *  PyroOS  -  paging (virtual memory)
 * ----------------------------------------------------------------------------
 *  The x86 paging unit translates every virtual address the CPU uses into a
 *  physical RAM address, using a two-level table:
 *
 *      virtual address ->  page directory (1024 entries)
 *                       ->  page table     (1024 entries)
 *                       ->  4 KB physical page
 *
 *  Each page table maps 1024 * 4 KB = 4 MB. We set up ONE page table that
 *  identity-maps the first 4 MB (virtual == physical), so all our existing
 *  code, stack, and VGA memory keep working unchanged. Then we point control
 *  register CR3 at the directory and flip the paging bit (CR0 bit 31).
 *
 *  With paging on, we can later map pages wherever we like -- the basis for
 *  per-process address spaces.
 * ==========================================================================*/
#include "paging.h"
#include <stdint.h>

#define PAGE_PRESENT 0x1
#define PAGE_RW      0x2
#define PAGE_USER    0x4    /* accessible from ring 3 (needed for user mode) */

/* The only range ring-3 code may touch: program load area + user stack. */
#define USER_ZONE_START 0x00080000u
#define USER_ZONE_END   0x00100000u

/* Page tables must be 4 KB aligned. These live in .bss (already zeroed). */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_page_table[1024] __attribute__((aligned(4096)));

/* A second table identity-maps 4 MB to 8 MB. That range is the physical frame
   pool, and the kernel must be able to write to a frame the moment it is
   allocated: a new page directory or page table is edited through its own
   physical address. Without this mapping the first vmm_create_dir faults. It
   is supervisor only, so ring 3 still cannot reach the pool. */
static uint32_t second_page_table[1024] __attribute__((aligned(4096)));

/* Exposed so the virtual memory manager can adopt this directory as the
   kernel address space rather than building a second one. */
uint32_t *paging_boot_directory(void)
{
    return page_directory;
}

void paging_install(void)
{
    /* Identity-map the first 4 MB. Only the user zone (0x80000..0xFFFFF) gets
       the USER flag; everything else (kernel code, data, heap) is supervisor-
       only, so ring-3 code that touches it page-faults. A page is reachable
       from ring 3 only when BOTH its table entry and the directory entry have
       the USER flag, so we set USER on the directory entry and selectively on
       the table entries. */
    for (int i = 0; i < 1024; i++) {
        uint32_t addr = (uint32_t)(i * 0x1000);
        uint32_t flags = PAGE_PRESENT | PAGE_RW;
        if (addr >= USER_ZONE_START && addr < USER_ZONE_END)
            flags |= PAGE_USER;
        first_page_table[i] = addr | flags;
    }

    /* Identity-map the frame pool, 4 MB to 8 MB, kernel only. */
    for (int i = 0; i < 1024; i++)
        second_page_table[i] = (0x00400000u + (uint32_t)(i * 0x1000))
                               | PAGE_PRESENT | PAGE_RW;

    /* Directory entry 0 covers 0 to 4 MB, entry 1 covers 4 to 8 MB. Every other
       entry stays zero, so memory above 8 MB is unmapped and faults on touch,
       which is exactly what demand paging needs in order to be triggered. */
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    page_directory[1] = ((uint32_t)second_page_table) | PAGE_PRESENT | PAGE_RW;

    /* Load the directory address into CR3. */
    __asm__ volatile("mov %0, %%cr3" :: "r"(page_directory));

    /* Set CR0 bit 31 (PG) to enable paging. */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}
