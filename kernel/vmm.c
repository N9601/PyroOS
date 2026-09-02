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
    /* Share the kernel's low mappings. Entry 0 keeps the kernel itself mapped
       so the CPU can keep fetching instructions after a switch. Entry 1 keeps
       the physical frame pool mapped, so the kernel can still edit this
       address space's page tables while it is the active one. */
    dir[0] = kernel_dir[0];
    dir[1] = kernel_dir[1];
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
    /* entries 0 and 1 are shared kernel mappings and are never freed */
    for (uint32_t di = 2; di < 1024; di++) {
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

/* ----------------------------------------------------------------------------
 *  vmm_clone_dir: a deep copy of an address space.
 *
 *  This is the memory half of fork. The child must see the same values at the
 *  same virtual addresses as the parent, but writing to one must not be visible
 *  in the other. So every private page gets a fresh frame with the parent's
 *  bytes copied into it, and the child's page tables point at the copies.
 *
 *  Directory entries 0 and 1 are shared kernel mappings and are aliased, not
 *  copied: entry 0 keeps the kernel mapped so execution survives the CR3 load,
 *  entry 1 keeps the frame pool mapped so the kernel can keep editing tables.
 *
 *  The copying itself is safe because every frame lives in the identity-mapped
 *  0 to 8 MB window, so a physical frame address is directly dereferenceable
 *  while the kernel directory is loaded. A later copy-on-write pass would map
 *  both sides read-only and duplicate only on the first write; this eager copy
 *  is the correct behaviour, just not yet the cheap one.
 * --------------------------------------------------------------------------*/
page_dir_t vmm_clone_dir(page_dir_t src)
{
    if (!src)
        return 0;

    page_dir_t dst = vmm_create_dir();      /* already shares entries 0 and 1 */
    if (!dst)
        return 0;

    for (uint32_t di = 2; di < 1024; di++) {
        if (!(src[di] & PF_PRESENT))
            continue;

        uint32_t tab_phys = pmm_alloc();    /* the child needs its own table */
        if (!tab_phys) {
            vmm_destroy_dir(dst);           /* out of memory: leave nothing behind */
            return 0;
        }
        uint32_t *src_tab = (uint32_t *)FRAME_OF(src[di]);
        uint32_t *dst_tab = (uint32_t *)tab_phys;
        memset(dst_tab, 0, PAGE_SIZE);
        dst[di] = tab_phys | (src[di] & 0xFFF);

        for (uint32_t ti = 0; ti < 1024; ti++) {
            uint32_t e = src_tab[ti];
            if (!(e & PF_PRESENT))
                continue;                   /* not-present entries carry a
                                               demand-paging promise, not data,
                                               and copy across as absent */
            uint32_t frame = pmm_alloc();
            if (!frame) {
                vmm_destroy_dir(dst);
                return 0;
            }
            memcpy((void *)frame, (void *)FRAME_OF(e), PAGE_SIZE);
            dst_tab[ti] = frame | (e & 0xFFF);
        }
    }
    return dst;
}

/* How many private frames an address space is holding: its page tables plus
   the pages they map. Used to show that a clone really did duplicate memory
   and that freeing it really does give the frames back. */
uint32_t vmm_dir_frames(page_dir_t dir)
{
    if (!dir)
        return 0;
    uint32_t n = 1;                         /* the directory frame itself */
    for (uint32_t di = 2; di < 1024; di++) {
        if (!(dir[di] & PF_PRESENT))
            continue;
        n++;                                /* the page table */
        uint32_t *tab = (uint32_t *)FRAME_OF(dir[di]);
        for (uint32_t ti = 0; ti < 1024; ti++)
            if (tab[ti] & PF_PRESENT)
                n++;
    }
    return n;
}

/* ----------------------------------------------------------------------------
 *  vmm_clone_cow: a copy-on-write clone of an address space.
 *
 *  Where vmm_clone_dir duplicates every data frame up front, this shares them.
 *  Parent and child point at the same physical pages, both marked read-only and
 *  tagged copy-on-write, and the frame's reference count is bumped. Nothing is
 *  copied until someone writes: the write faults, the fault handler makes a
 *  private copy for the writer, and the two diverge one page at a time.
 *
 *  Page tables are still copied, not shared. Two processes must be able to hold
 *  different permissions for the same page (that is the whole mechanism), and
 *  permissions live in the page-table entry, so the entries cannot be shared
 *  even though the frames they point at are.
 *
 *  The parent's own entries are made read-only too. If only the child's were,
 *  the parent could write the shared page and the child would see the change,
 *  which is exactly the isolation COW exists to prevent.
 * --------------------------------------------------------------------------*/
page_dir_t vmm_clone_cow(page_dir_t src)
{
    if (!src)
        return 0;

    page_dir_t dst = vmm_create_dir();      /* shares kernel entries 0 and 1 */
    if (!dst)
        return 0;

    for (uint32_t di = 2; di < 1024; di++) {
        if (!(src[di] & PF_PRESENT))
            continue;

        uint32_t tab_phys = pmm_alloc();
        if (!tab_phys) {
            vmm_destroy_dir(dst);
            return 0;
        }
        uint32_t *src_tab = (uint32_t *)FRAME_OF(src[di]);
        uint32_t *dst_tab = (uint32_t *)tab_phys;
        memset(dst_tab, 0, PAGE_SIZE);
        dst[di] = tab_phys | (src[di] & 0xFFF);

        for (uint32_t ti = 0; ti < 1024; ti++) {
            uint32_t e = src_tab[ti];
            if (!(e & PF_PRESENT))
                continue;

            uint32_t frame = FRAME_OF(e);

            /* Drop write permission, tag as copy-on-write, on both sides. A
               page that was already read-only stays read-only and is simply
               shared; there is nothing to copy on a write that cannot happen. */
            uint32_t shared = (e & ~PF_RW) | (e & PF_RW ? PF_COW : 0);
            src_tab[ti] = shared;
            dst_tab[ti] = shared;
            pmm_incref(frame);          /* now held by two address spaces */
        }
    }
    return dst;
}
