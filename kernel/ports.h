/* ============================================================================
 *  PyroOS  -  x86 I/O port access
 * ----------------------------------------------------------------------------
 *  Hardware like the interrupt controller (PIC) and the keyboard are reached
 *  through "I/O ports" -- a separate address space from memory, accessed with
 *  the special in/out CPU instructions. These inline-assembly wrappers let C
 *  read and write those ports.
 * ==========================================================================*/
#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

/* Read one byte from an I/O port. */
static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* Write one byte to an I/O port. */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Read one 16-bit word (used for the ATA data register). */
static inline uint16_t inw(uint16_t port)
{
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/* Write one 16-bit word. */
static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

#endif
