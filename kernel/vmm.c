/* ============================================================================
 *  PyroOS  -  virtual memory manager
 * ----------------------------------------------------------------------------
 *  A page directory holds 1024 entries, each pointing at a page table that maps
 *  4 MB. Entry 0 covers the first 4 MB, which is where the kernel lives, so
 *  every address space shares that entry: a process switch must not unmap the
 *  kernel out from under the CPU that is executing it.
 *
 *  Everything above 4 MB is private to the address space. Two processes can
 *  both use virtual address 0x00400000 and get different physical frames.
 *
 *  Note on self-reference: because the first 4 MB is identity mapped, a
 *  physical address below 4 MB is also a valid pointer, so page tables
 *  allocated from that region can be edited directly. Tables allocated above
 *  it are reached through the identity map of their own frame, which holds
 *  while the kernel directory is active.
 * ==========================================================================*/
#include "vmm.h"
#include "pmm.h"
#include "string.h"

#define DIR_INDEX(v)  (((v) >> 22) & 0x3FF)
#define TAB_INDEX(v)  (((v) >> 12) & 0x3FF)
#define FRAME_OF(e)   ((e) & 0xFFFFF000u)

extern uint32_t *paging_boot_directory(void);   /* from paging.c */

static page_dir_t kernel_dir;

void vmm_install(void)
{
    kernel_dir = paging_boot_directory();
}

page_dir_t vmm_kernel_dir(void)
{
    return kernel_dir;
}

void vmm_flush(uint32_t virt)
{
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void vmm_switch(page_dir_t dir)
{
    __asm__ volatile("mov %0, %%cr3" :: "r"((uint32_t)dir) : "memory");
}

page_dir_t vmm_create_dir(void)
{
    uint32_t phys = pmm_alloc();
    if (!phys)
        return 0;
    page_dir_t dir = (page_dir_t)phys;
    memset(dir, 0, PAGE_SIZE);
    /* share the kernel's first 4 MB, so the kernel stays mapped after a switch */
    dir[0] = kernel_dir[0];
    return dir;
}

static uint32_t *table_for(page_dir_t dir, uint32_t virt, int create, uint32_t flags)
{
    uint32_t di = DIR_INDEX(virt);
    if (!(dir[di] & PF_PRESENT)) {
        if (!create)
            return 0;
        uint32_t phys = pmm_alloc();
        if (!phys)
            return 0;
        memset((void *)phys, 0, PAGE_SIZE);
        dir[di] = phys | PF_PRESENT | PF_RW | (flags & PF_USER);
    }
    return (uint32_t *)FRAME_OF(dir[di]);
}

int vmm_map(page_dir_t dir, uint32_t virt, uint32_t phys, uint32_t flags)
{
    uint32_t *tab = table_for(dir, virt, 1, flags);
    if (!tab)
        return -1;
    tab[TAB_INDEX(virt)] = FRAME_OF(phys) | (flags & 0xFFF) | PF_PRESENT;
    vmm_flush(virt);
    return 0;
}

int vmm_map_alloc(page_dir_t dir, uint32_t virt, uint32_t flags)
{
    uint32_t phys = pmm_alloc();
    if (!phys)
        return -1;
    memset((void *)phys, 0, PAGE_SIZE);     /* never hand a process stale data */
    return vmm_map(dir, virt, phys, flags);
}

uint32_t vmm_translate(page_dir_t dir, uint32_t virt)
{
    uint32_t *tab = table_for(dir, virt, 0, 0);
    if (!tab)
        return 0;
    uint32_t e = tab[TAB_INDEX(virt)];
    if (!(e & PF_PRESENT))
        return 0;
    return FRAME_OF(e) | (virt & 0xFFF);
}

void vmm_unmap(page_dir_t dir, uint32_t virt)
{
    uint32_t *tab = table_for(dir, virt, 0, 0);
    if (!tab)
        return;
    uint32_t e = tab[TAB_INDEX(virt)];
    if (e & PF_PRESENT) {
        pmm_free(FRAME_OF(e));
        tab[TAB_INDEX(virt)] = 0;
        vmm_flush(virt);
    }
}

void vmm_destroy_dir(page_dir_t dir)
{
    if (!dir || dir == kernel_dir)
        return;
    /* entry 0 is the shared kernel mapping and is never freed */
    for (uint32_t di = 1; di < 1024; di++) {
        if (!(dir[di] & PF_PRESENT))
            continue;
        uint32_t *tab = (uint32_t *)FRAME_OF(dir[di]);
        for (uint32_t ti = 0; ti < 1024; ti++) {
            if (tab[ti] & PF_PRESENT)
                pmm_free(FRAME_OF(tab[ti]));
        }
        pmm_free(FRAME_OF(dir[di]));
        dir[di] = 0;
    }
    pmm_free((uint32_t)dir);
}
