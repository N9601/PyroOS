/* ============================================================================
 *  PyroOS  -  pixel-art flame logo
 * ----------------------------------------------------------------------------
 *  Drawn with the CP437 full-block glyph (0xDB) in VGA text memory: each block
 *  is one "pixel" in a fire palette. The flame bitmap below uses one letter per
 *  pixel; blanks are skipped. This works entirely in text mode, so it does not
 *  disturb the shell.
 *
 *      w = white   y = yellow   o = orange   R = bright red   r = dark red
 * ==========================================================================*/
#include "logo.h"
#include "screen.h"
#include "timer.h"

#define BLOCK 0xDB

static const char *flame[] = {
    "        w        ",
    "       www       ",
    "       wyw       ",
    "      wwyww      ",
    "      wyyyw      ",
    "     wyyoyyw     ",
    "     yyoooyy     ",
    "    yyoRRRoyy    ",
    "   yyoRRRRRoyy   ",
    "   yoRRRRRRRoy   ",
    "  yoRRRrrrRRRoy  ",
    "  oRRRrrrrrRRRo  ",
    " RRRRrrrrrrrRRRR ",
    " RRRrrrrrrrrrRRR ",
    "  RRrrrrrrrrrRR  ",
    "   RrrrrrrrrrR   ",
    "    rrrrrrrrr    ",
};
#define NROWS ((int)(sizeof(flame) / sizeof(flame[0])))

static uint8_t color_of(char p)
{
    switch (p) {
    case 'w': return 0x0F;   /* white */
    case 'y': return 0x0E;   /* yellow */
    case 'o': return 0x06;   /* orange/brown */
    case 'R': return 0x0C;   /* bright red */
    case 'r': return 0x04;   /* dark red */
    default:  return 0;      /* blank */
    }
}

static void draw_string(int row, int col, const char *s, uint8_t color)
{
    for (int i = 0; s[i]; i++)
        screen_put(row, col + i, (unsigned char)s[i], color);
}

void draw_logo(void)
{
    clear_screen();

    const int base_row = 2;
    const int base_col = 31;

    for (int r = 0; r < NROWS; r++) {
        const char *row = flame[r];
        for (int c = 0; row[c]; c++) {
            uint8_t col = color_of(row[c]);
            if (col)
                screen_put(base_row + r, base_col + c, BLOCK, col);
        }
    }

    draw_string(base_row + NROWS + 1, (80 - 11) / 2, "P Y R O O S", 0x0E);
    draw_string(base_row + NROWS + 2, (80 - 33) / 2,
                "a from-scratch x86 operating system", 0x07);
}

void logo_splash(uint32_t ticks)
{
    draw_logo();
    uint32_t start = timer_ticks();
    while (timer_ticks() - start < ticks)
        __asm__ volatile("hlt");
    clear_screen();
}
