/* ============================================================================
 *  PyroOS  -  kernel GDT with user segments and a TSS
 * ----------------------------------------------------------------------------
 *  The bootloader set up a minimal GDT (null, kernel code, kernel data). To
 *  run user-mode code we need two more segments at privilege level 3, plus a
 *  Task State Segment (TSS). We keep kernel code/data at 0x08/0x10 so all
 *  existing selectors stay valid, and reload the GDT.
 *
 *  The TSS's only job here is to hold the ring-0 stack pointer (ss0:esp0). When
 *  a ring-3 program makes a syscall or is interrupted, the CPU switches to that
 *  stack automatically. Get this wrong and the first trap from ring 3 triple-
 *  faults.
 * ==========================================================================*/
#include "gdt.h"
#include "string.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0, ss0;
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs, ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr   gp;
static struct tss_entry tss;
static uint8_t kernel_stack[8192] __attribute__((aligned(16)));

extern void gdt_flush(uint32_t gdt_ptr);
extern void tss_flush(void);

static void set_gate(int n, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[n].base_low  = base & 0xFFFF;
    gdt[n].base_mid  = (base >> 16) & 0xFF;
    gdt[n].base_high = (base >> 24) & 0xFF;
    gdt[n].limit_low = limit & 0xFFFF;
    gdt[n].gran      = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[n].access    = access;
}

void tss_set_stack(uint32_t esp0)
{
    tss.esp0 = esp0;
}

void gdt_install(void)
{
    set_gate(0, 0, 0, 0, 0);                     /* null */
    set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);         /* kernel code (ring 0) */
    set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);         /* kernel data (ring 0) */
    set_gate(3, 0, 0xFFFFF, 0xFA, 0xCF);         /* user code   (ring 3) */
    set_gate(4, 0, 0xFFFFF, 0xF2, 0xCF);         /* user data   (ring 3) */

    /* TSS descriptor: access 0x89 = present, DPL 0, 32-bit TSS (available). */
    uint32_t base  = (uint32_t)&tss;
    uint32_t limit = sizeof(tss) - 1;
    set_gate(5, base, limit, 0x89, 0x00);

    memset(&tss, 0, sizeof(tss));
    tss.ss0  = KERNEL_DATA;                       /* ring-0 stack segment */
    tss.esp0 = (uint32_t)&kernel_stack[sizeof(kernel_stack)];
    tss.iomap_base = sizeof(tss);

    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_flush((uint32_t)&gp);
    tss_flush();
}
