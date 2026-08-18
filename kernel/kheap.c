/* ============================================================================
 *  PyroOS  -  kernel heap (dynamic memory)
 * ----------------------------------------------------------------------------
 *  A simple first-fit free-list allocator over a fixed 3 MB region (1 MB..4 MB,
 *  inside the identity-mapped range). Each block carries a small header:
 *
 *      [ header | usable bytes ][ header | usable bytes ] ...
 *
 *  kmalloc walks the list for the first free block big enough, splitting it if
 *  there's leftover room. kfree marks a block free and merges it with the next
 *  block if that one is also free (coalescing), which limits fragmentation.
 * ==========================================================================*/
#include "kheap.h"

#define HEAP_START 0x00100000u          /* 1 MB */
#define HEAP_SIZE  0x00300000u          /* 3 MB, ending at the 4 MB map boundary */

typedef struct block {
    uint32_t size;          /* usable bytes in this block (excludes header) */
    uint32_t free;          /* 1 = free, 0 = in use */
    struct block *next;     /* next block in the list */
} block_t;

static block_t *heap_head = 0;

void heap_install(void)
{
    heap_head = (block_t *)HEAP_START;
    heap_head->size = HEAP_SIZE - sizeof(block_t);
    heap_head->free = 1;
    heap_head->next = 0;
}

void *kmalloc(uint32_t size)
{
    size = (size + 3u) & ~3u;            /* round up to a 4-byte boundary */

    for (block_t *cur = heap_head; cur; cur = cur->next) {
        if (cur->free && cur->size >= size) {
            /* Split off the remainder if there's room for another header. */
            if (cur->size >= size + sizeof(block_t) + 4u) {
                block_t *rest = (block_t *)((uint8_t *)cur + sizeof(block_t) + size);
                rest->size = cur->size - size - sizeof(block_t);
                rest->free = 1;
                rest->next = cur->next;
                cur->size = size;
                cur->next = rest;
            }
            cur->free = 0;
            return (uint8_t *)cur + sizeof(block_t);
        }
    }
    return 0;                            /* out of memory */
}

void kfree(void *ptr)
{
    if (!ptr)
        return;

    block_t *b = (block_t *)((uint8_t *)ptr - sizeof(block_t));
    b->free = 1;

    /* Coalesce with the following block if it is also free. */
    if (b->next && b->next->free) {
        b->size += sizeof(block_t) + b->next->size;
        b->next = b->next->next;
    }
}
