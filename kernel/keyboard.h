/* ============================================================================
 *  PyroOS  -  PS/2 keyboard driver
 * ==========================================================================*/
#ifndef KEYBOARD_H
#define KEYBOARD_H

/* Special codes returned by keyboard_getchar for the arrow keys (they are not
   printable ASCII, so we use unused control codes). */
#define KEY_UP    0x11
#define KEY_DOWN  0x12
#define KEY_LEFT  0x13
#define KEY_RIGHT 0x14

void keyboard_install(void);   /* register the IRQ1 handler */
int  keyboard_getchar(void);   /* next buffered character, or -1 if none */

#endif
