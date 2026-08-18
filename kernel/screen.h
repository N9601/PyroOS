/* ============================================================================
 *  PyroOS  -  screen driver (VGA text mode)
 * ==========================================================================*/
#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#define COLOR_WHITE_ON_BLACK 0x0F
#define COLOR_GREEN_ON_BLACK 0x0A
#define COLOR_RED_ON_BLACK   0x0C

void clear_screen(void);
void kprint_char(char c);                       /* print one char at the cursor */
void kprint(const char *s);                     /* print a string */
void kprint_color(const char *s, uint8_t color);/* print a string in a color */
void kprint_hex(uint32_t n);                    /* print a number as 0xXXXXXXXX */
void kprint_dec(uint32_t n);                    /* print a number in decimal */
void screen_put(int row, int col, unsigned char c, uint8_t color); /* one cell, no cursor move */

#endif
