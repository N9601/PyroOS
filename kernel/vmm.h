/* ============================================================================
 *  PyroOS  -  virtual memory manager
 * ----------------------------------------------------------------------------
 *  Builds and edits page directories. This is what makes a per-process address
 *  space possible: each process gets its own directory, so the same virtual
 *  address can mean different physical memory in different processes.
 * ==========================================================================*/
#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PF_PRESENT 0x1
#define PF_RW      0x2
#define PF_USER    0x4

typedef uint32_t *page_dir_t;      /* 1024 entries, 4 KB aligned */

void        vmm_install(void);                 /* take over the boot directory */
page_dir_t  vmm_kernel_dir(void);

page_dir_t  vmm_create_dir(void);              /* new address space, kernel shared */
void        vmm_destroy_dir(page_dir_t dir);   /* free its private frames */
void        vmm_switch(page_dir_t dir);        /* load CR3 */

int         vmm_map(page_dir_t dir, uint32_t virt, uint32_t phys, uint32_t flags);
int         vmm_map_alloc(page_dir_t dir, uint32_t virt, uint32_t flags);
uint32_t    vmm_translate(page_dir_t dir, uint32_t virt);   /* 0 if unmapped */
void        vmm_unmap(page_dir_t dir, uint32_t virt);
page_dir_t  vmm_clone_dir(page_dir_t src);     /* deep copy: the memory half of fork */
uint32_t    vmm_dir_frames(page_dir_t dir);    /* private frames it holds */
void        vmm_flush(uint32_t virt);

#endif
