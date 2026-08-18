/* ============================================================================
 *  PyroOS  -  screen driver (VGA text mode)
 * ----------------------------------------------------------------------------
 *  Writes text to VGA memory at 0xB8000 with a software cursor that advances
 *  as we print, handles newlines and backspace, and scrolls when the screen
 *  fills up. This is what our interrupt handlers use to echo keystrokes.
 * ==========================================================================*/
#include "screen.h"

#define VIDEO_MEMORY 0xB8000
#define COLS 80
#define ROWS 25

static volatile uint8_t *const video = (volatile uint8_t *)VIDEO_MEMORY;
static int cursor = 0;                   /* current cell index, 0 .. COLS*ROWS-1 */
static uint8_t text_color = COLOR_WHITE_ON_BLACK;

static void put_cell(int index, char c, uint8_t color)
{
    video[index * 2]     = (uint8_t)c;   /* character byte */
    video[index * 2 + 1] = color;        /* attribute (color) byte */
}

/* When the cursor runs off the bottom, shift every row up by one and clear
   the last row -- classic terminal scrolling. */
static void scroll_if_needed(void)
{
    if (cursor < COLS * ROWS)
        return;

    for (int i = 0; i < COLS * (ROWS - 1); i++) {
        video[i * 2]     = video[(i + COLS) * 2];
        video[i * 2 + 1] = video[(i + COLS) * 2 + 1];
    }
    for (int i = COLS * (ROWS - 1); i < COLS * ROWS; i++)
        put_cell(i, ' ', text_color);

    cursor = COLS * (ROWS - 1);
}

void clear_screen(void)
{
    for (int i = 0; i < COLS * ROWS; i++)
        put_cell(i, ' ', COLOR_WHITE_ON_BLACK);
    cursor = 0;
}

void kprint_char(char c)
{
    if (c == '\n') {
        cursor = (cursor / COLS + 1) * COLS;     /* jump to start of next line */
    } else if (c == '\b') {
        if (cursor > 0) {
            cursor--;
            put_cell(cursor, ' ', text_color);   /* erase the character */
        }
    } else {
        put_cell(cursor, c, text_color);
        cursor++;
    }
    scroll_if_needed();
}

void kprint_color(const char *s, uint8_t color)
{
    uint8_t saved = text_color;
    text_color = color;
    for (int i = 0; s[i] != '\0'; i++)
        kprint_char(s[i]);
    text_color = saved;
}

void kprint(const char *s)
{
    for (int i = 0; s[i] != '\0'; i++)
        kprint_char(s[i]);
}

void kprint_hex(uint32_t n)
{
    const char *digits = "0123456789abcdef";
    kprint("0x");
    for (int shift = 28; shift >= 0; shift -= 4)
        kprint_char(digits[(n >> shift) & 0xF]);
}

void screen_put(int row, int col, unsigned char c, uint8_t color)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return;
    put_cell(row * COLS + col, (char)c, color);
}

void kprint_dec(uint32_t n)
{
    char buf[11];
    int i = 10;
    buf[i--] = '\0';
    if (n == 0) {
        kprint_char('0');
        return;
    }
    while (n > 0 && i >= 0) {
        buf[i--] = '0' + (n % 10);
        n /= 10;
    }
    kprint(&buf[i + 1]);
}
