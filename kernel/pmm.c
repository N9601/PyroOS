/* ============================================================================
 *  PyroOS  -  physical memory manager
 * ----------------------------------------------------------------------------
 *  A bitmap allocator: one bit per 4 KB frame of physical RAM, set when the
 *  frame is in use. Allocation scans for the first clear bit, which is simple
 *  and predictable; a free list would be faster but harder to verify, and this
 *  is not on a hot path.
 *
 *  Frames below MANAGED_BASE are reserved permanently: they hold the boot
 *  sector, the kernel image, the kernel stack, the kernel heap and the VGA
 *  buffer, none of which may ever be handed out.
 * ==========================================================================*/
#include "pmm.h"
#include "string.h"

/* Everything under 4 MB already belongs to the kernel (code, heap, user zone,
   identity-mapped hardware). Frames handed out start above that. */
#define MANAGED_BASE 0x00400000u

/* One bit per frame. 32768 bits covers 128 MB of managed RAM. */
#define MAX_FRAMES 32768u
#define BITMAP_WORDS (MAX_FRAMES / 32u)

static uint32_t bitmap[BITMAP_WORDS];
static uint32_t total_frames;
static uint32_t used_frames;
static uint32_t next_hint;          /* where the last search stopped */

static inline void bit_set(uint32_t i)   { bitmap[i >> 5] |= (1u << (i & 31)); }
static inline void bit_clear(uint32_t i) { bitmap[i >> 5] &= ~(1u << (i & 31)); }
static inline int  bit_test(uint32_t i)  { return (bitmap[i >> 5] >> (i & 31)) & 1u; }

void pmm_install(uint32_t mem_bytes)
{
    memset(bitmap, 0, sizeof(bitmap));
    used_frames = 0;
    next_hint = 0;

    if (mem_bytes <= MANAGED_BASE) {
        total_frames = 0;
        return;
    }
    uint32_t frames = (mem_bytes - MANAGED_BASE) / PAGE_SIZE;
    total_frames = frames > MAX_FRAMES ? MAX_FRAMES : frames;
}

uint32_t pmm_alloc(void)
{
    if (total_frames == 0)
        return 0;

    for (uint32_t pass = 0; pass < 2; pass++) {
        uint32_t start = pass == 0 ? next_hint : 0;
        uint32_t end = pass == 0 ? total_frames : next_hint;
        for (uint32_t i = start; i < end; i++) {
            if (!bit_test(i)) {
                bit_set(i);
                used_frames++;
                next_hint = i + 1;
                return MANAGED_BASE + i * PAGE_SIZE;
            }
        }
    }
    return 0;                       /* out of physical memory */
}

void pmm_free(uint32_t phys)
{
    if (phys < MANAGED_BASE)
        return;                     /* never reclaim kernel-reserved frames */
    uint32_t i = (phys - MANAGED_BASE) / PAGE_SIZE;
    if (i >= total_frames || !bit_test(i))
        return;                     /* out of range, or a double free */
    bit_clear(i);
    used_frames--;
    if (i < next_hint)
        next_hint = i;
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames; }
uint32_t pmm_free_frames(void)  { return total_frames - used_frames; }
