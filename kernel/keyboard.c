/* ============================================================================
 *  PyroOS  -  PS/2 keyboard driver, IRQ1
 * ----------------------------------------------------------------------------
 *  On each key event the keyboard fires IRQ1. We read a scancode from port
 *  0x60; the high bit means "released" (ignored). Otherwise we translate it to
 *  ASCII and push it into a ring buffer. The shell drains that buffer with
 *  keyboard_getchar(). Keeping the interrupt handler tiny (just enqueue) and
 *  doing the real work outside it is good practice.
 *
 *  Single-producer (IRQ) / single-consumer (main loop) ring buffer: the
 *  producer only advances `head`, the consumer only advances `tail`, and
 *  32-bit reads/writes are atomic on x86, so no locking is needed.
 * ==========================================================================*/
#include "keyboard.h"
#include "isr.h"
#include "ports.h"

#include <stdint.h>

#define KEYBOARD_DATA_PORT 0x60
#define BUF_SIZE 256

static const char scancode_ascii[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o',
    'p', '[', ']', '\n',
    0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'','`',
    0,
    '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.',
    '/',
    0,
    '*',
    0,
    ' ',
};

/* The same keys with Shift held: symbols and capital letters. */
static const char scancode_ascii_shift[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O',
    'P', '{', '}', '\n',
    0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    '"', '~',
    0,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>',
    '?',
    0,
    '*',
    0,
    ' ',
};

#define LSHIFT 0x2A
#define RSHIFT 0x36

static volatile char     buffer[BUF_SIZE];
static volatile uint32_t head = 0;   /* written only by the IRQ (producer) */
static volatile uint32_t tail = 0;   /* written only by the consumer */
static volatile int      shift = 0;  /* is a Shift key currently held */

static void keyboard_callback(registers_t *r)
{
    (void)r;
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    if (scancode & 0x80) {                      /* key release */
        uint8_t code = scancode & 0x7F;
        if (code == LSHIFT || code == RSHIFT)
            shift = 0;
        return;
    }

    if (scancode == LSHIFT || scancode == RSHIFT) {
        shift = 1;                              /* Shift pressed */
        return;
    }

    char c = shift ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
    if (!c)
        return;

    uint32_t next = (head + 1) % BUF_SIZE;
    if (next != tail) {                         /* drop if the buffer is full */
        buffer[head] = c;
        head = next;
    }
}

int keyboard_getchar(void)
{
    if (tail == head)
        return -1;                              /* nothing buffered */
    char c = buffer[tail];
    tail = (tail + 1) % BUF_SIZE;
    return (unsigned char)c;
}

void keyboard_install(void)
{
    register_interrupt_handler(33, keyboard_callback);  /* IRQ1 -> vector 33 */
}
