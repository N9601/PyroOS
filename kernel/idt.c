/* ============================================================================
 *  PyroOS  -  Interrupt Descriptor Table (IDT) setup
 * ==========================================================================*/
#include "idt.h"
#include "isr.h"

struct idt_entry idt[256];      /* the table itself (lives in .bss, zeroed) */
struct idt_ptr   idtp;          /* the pointer we feed to lidt */

extern void idt_flush(uint32_t);/* defined in interrupt.asm: runs lidt */

/* Fill one IDT entry with a handler address and flags. */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_install(void)
{
    idtp.limit = sizeof(struct idt_entry) * 256 - 1;
    idtp.base  = (uint32_t)&idt;

    /* Start with every gate empty. */
    for (int i = 0; i < 256; i++)
        idt_set_gate(i, 0, 0, 0);

    /* isr_install (in isr.c) fills in the CPU-exception and hardware-IRQ
       gates and remaps the PIC. */
    isr_install();

    /* Load the table into the CPU. */
    idt_flush((uint32_t)&idtp);
}
