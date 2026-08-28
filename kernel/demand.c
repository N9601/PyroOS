/* ============================================================================
 *  PyroOS  -  demand paging
 * ----------------------------------------------------------------------------
 *  A region can be promised without committing any physical memory to it. The
 *  addresses are legal, but no page tables are filled in, so the first touch
 *  raises a page fault. The handler asks this module whether the address was
 *  promised; if it was, a frame is allocated and mapped and the faulting
 *  instruction simply runs again.
 *
 *  This is why a system can offer a process far more address space than the
 *  machine has RAM: memory is only spent on pages that are actually used.
 * ==========================================================================*/
#include "demand.h"
#include "pmm.h"
#include "vmm.h"

#define MAX_REGIONS 16

typedef struct {
    page_dir_t dir;
    uint32_t   start, end, flags;
    int        active;
} region_t;

static region_t regions[MAX_REGIONS];
static uint32_t faults_served;

void demand_region(page_dir_t dir, uint32_t start, uint32_t end, uint32_t flags)
{
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (!regions[i].active) {
            regions[i].dir = dir;
            regions[i].start = start & ~0xFFFu;
            regions[i].end = (end + 0xFFFu) & ~0xFFFu;
            regions[i].flags = flags;
            regions[i].active = 1;
            return;
        }
    }
}

void demand_clear(void)
{
    for (int i = 0; i < MAX_REGIONS; i++)
        regions[i].active = 0;
    faults_served = 0;
}

uint32_t demand_faults_served(void) { return faults_served; }

int demand_handle(uint32_t addr, uint32_t err_code)
{
    /* Bit 0 of the error code set means the page was present, so this is a
       protection violation (a write to a read-only page, or ring 3 touching a
       supervisor page). Those are real errors, not missing pages. */
    if (err_code & 0x1)
        return 0;

    /* Which address space faulted: whatever CR3 currently points at. */
    uint32_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    page_dir_t current = (page_dir_t)(cr3 & 0xFFFFF000u);

    for (int i = 0; i < MAX_REGIONS; i++) {
        region_t *r = &regions[i];
        if (!r->active || r->dir != current)
            continue;
        if (addr < r->start || addr >= r->end)
            continue;
        if (vmm_map_alloc(r->dir, addr & ~0xFFFu, r->flags) != 0)
            return 0;               /* out of physical memory: let it fault */
        faults_served++;
        return 1;                   /* resolved, resume the instruction */
    }
    return 0;                       /* address was never promised */
}
