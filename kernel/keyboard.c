/* ============================================================================
 *  PyroOS  -  PS/2 keyboard driver, IRQ1
 * ----------------------------------------------------------------------------
 *  Each key press and release makes the keyboard controller fire IRQ1. We read
 *  a "scancode" from port 0x60. The high bit (0x80) means "key released"; we
 *  ignore releases. Otherwise we translate the scancode to an ASCII character
 *  using a US-QWERTY table and echo it to the screen.
 *
 *  This is scan code set 1 (the PC/AT default in QEMU). Only the common keys
 *  are mapped; unmapped keys produce 0 and are skipped.
 * ==========================================================================*/
#include "keyboard.h"
#include "isr.h"
#include "ports.h"
#include "screen.h"

#include <stdint.h>

#define KEYBOARD_DATA_PORT 0x60

/* scancode -> ASCII (unshifted US layout). Index is the scancode. */
static const char scancode_ascii[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8',   /* 0x00-0x09 */
    '9', '0', '-', '=', '\b',                            /* 0x0A-0x0E (backspace) */
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',    /* 0x0F-0x18 (tab) */
    'p', '[', ']', '\n',                                 /* 0x19-0x1C (enter) */
    0,                                                   /* 0x1D left control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    /* 0x1E-0x27 */
    '\'','`',                                            /* 0x28-0x29 */
    0,                                                   /* 0x2A left shift */
    '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',    /* 0x2B-0x34 */
    '/',                                                 /* 0x35 */
    0,                                                   /* 0x36 right shift */
    '*',                                                 /* 0x37 keypad * */
    0,                                                   /* 0x38 left alt */
    ' ',                                                 /* 0x39 space */
    /* remaining entries default to 0 */
};

static void keyboard_callback(registers_t *r)
{
    (void)r;
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode & 0x80)
        return;                         /* key release: ignore */

    char c = scancode_ascii[scancode & 0x7F];
    if (c)
        kprint_char(c);                 /* echo the typed character */
}

void keyboard_install(void)
{
    register_interrupt_handler(33, keyboard_callback);  /* IRQ1 -> vector 33 */
}
