/* ============================================================================
 *  PyroOS  -  Interrupt Descriptor Table (IDT)
 * ----------------------------------------------------------------------------
 *  The IDT maps each of the 256 possible interrupt numbers to a handler
 *  address. When interrupt N fires, the CPU looks up entry N here and jumps to
 *  the address stored in it.
 * ==========================================================================*/
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* One 8-byte IDT entry. The handler address is split into low and high halves
   for historical reasons, just like the GDT. __attribute__((packed)) stops the
   compiler from inserting padding, which would corrupt the layout. */
struct idt_entry {
    uint16_t base_low;      /* handler address bits 0-15 */
    uint16_t sel;           /* code segment selector (our 0x08) */
    uint8_t  always0;       /* reserved, always zero */
    uint8_t  flags;         /* type + privilege (0x8E = present, ring 0, 32-bit) */
    uint16_t base_high;     /* handler address bits 16-31 */
} __attribute__((packed));

/* The value we hand to the lidt instruction: table size and location. */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_install(void);

#endif
