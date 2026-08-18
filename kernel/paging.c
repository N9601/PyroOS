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

/* Page tables must be 4 KB aligned. These live in .bss (already zeroed). */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void paging_install(void)
{
    /* Identity-map the first 4 MB: page i covers physical address i*4KB. The
       USER flag lets ring-3 code run here too (see the user-mode milestone;
       real isolation would give user code its own separate pages). */
    for (int i = 0; i < 1024; i++)
        first_page_table[i] = (i * 0x1000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;

    /* Directory entry 0 points at that table. All other directory entries stay
       zero (not present), so touching memory above 4 MB will page-fault. */
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW | PAGE_USER;

    /* Load the directory address into CR3. */
    __asm__ volatile("mov %0, %%cr3" :: "r"(page_directory));

    /* Set CR0 bit 31 (PG) to enable paging. */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));
}
