/* ============================================================================
 *  PyroOS  -  a user-space heap allocator
 * ----------------------------------------------------------------------------
 *  A first-fit free list built on the SYS_SBRK system call. This is the layer
 *  a C program expects under malloc: sbrk grows the heap in coarse chunks, and
 *  this hands out and recycles pieces of it.
 *
 *  Header-only and static, so a standalone user program just includes it. Each
 *  program is a single translation unit, so its free list is private to it,
 *  which is correct: heaps are not shared between processes.
 *
 *  Every block carries a small header: its payload size and a free flag, plus a
 *  link to the next block in address order. Freed blocks are coalesced with
 *  their neighbour so that a run of free/alloc/free does not fragment the heap
 *  into unusable slivers.
 * ==========================================================================*/
#ifndef UMALLOC_H
#define UMALLOC_H

#define SYS_SBRK 9

typedef unsigned int u_size_t;

static void *u_sbrk(int incr)
{
    void *old;
    __asm__ volatile("int $0x80" : "=a"(old) : "a"(SYS_SBRK), "b"(incr));
    return old;                     /* (void *)-1 on failure */
}

typedef struct u_block {
    u_size_t        size;           /* payload bytes, not counting this header */
    int             free;
    struct u_block *next;
} u_block_t;

static u_block_t *u_head = 0;       /* first block; the list is address-ordered */

#define U_HDR sizeof(u_block_t)

/* Fold a just-freed block into the next one if that is also free. Called after
   every free, so at most one merge is ever pending. */
static void u_coalesce(void)
{
    for (u_block_t *b = u_head; b && b->next; b = b->next) {
        if (b->free && b->next->free) {
            b->size += U_HDR + b->next->size;
            b->next = b->next->next;
        }
    }
}

static void *umalloc(u_size_t want)
{
    if (want == 0)
        return 0;
    want = (want + 7u) & ~7u;       /* keep payloads 8-byte aligned */

    /* First fit: the earliest free block big enough. */
    for (u_block_t *b = u_head; b; b = b->next) {
        if (b->free && b->size >= want) {
            /* Split if the leftover is worth its own header. */
            if (b->size >= want + U_HDR + 8) {
                u_block_t *rest = (u_block_t *)((char *)(b + 1) + want);
                rest->size = b->size - want - U_HDR;
                rest->free = 1;
                rest->next = b->next;
                b->size = want;
                b->next = rest;
            }
            b->free = 0;
            return b + 1;
        }
    }

    /* Nothing fit: grow the heap by one block via sbrk. */
    u_block_t *b = (u_block_t *)u_sbrk((int)(U_HDR + want));
    if (b == (void *)-1)
        return 0;                   /* out of heap */
    b->size = want;
    b->free = 0;
    b->next = 0;
    if (!u_head) {
        u_head = b;
    } else {
        u_block_t *t = u_head;
        while (t->next)
            t = t->next;
        t->next = b;
    }
    return b + 1;
}

static void ufree(void *p)
{
    if (!p)
        return;
    u_block_t *b = (u_block_t *)p - 1;
    b->free = 1;
    u_coalesce();
}

#endif
