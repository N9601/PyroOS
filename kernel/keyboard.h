/* ============================================================================
 *  PyroOS  -  PS/2 keyboard driver
 * ==========================================================================*/
#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_install(void);   /* register the IRQ1 handler */
int  keyboard_getchar(void);   /* next buffered character, or -1 if none */

#endif
