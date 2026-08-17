/* ============================================================================
 *  PyroOS  -  the C kernel
 * ----------------------------------------------------------------------------
 *  This is the first PyroOS code written in C instead of assembly. The boot
 *  sector loads this from disk into memory and, once the CPU is in 32-bit
 *  protected mode, jumps to it.
 *
 *  There is NO standard library here. No printf, no malloc, no operating
 *  system underneath -- WE are the operating system. All we can do is talk to
 *  hardware directly. So to put text on screen we write bytes straight into
 *  VGA text memory at 0xB8000, exactly like our assembly print_pm did.
 *
 *  The screen is 80 columns x 25 rows. Each cell is 2 bytes: the character,
 *  then a color attribute. 0x0A is light-green on black.
 * ==========================================================================*/

#define VIDEO_MEMORY 0xB8000
#define COLS 80
#define GREEN_ON_BLACK 0x0A

/* Print a null-terminated string at a given (row, col) on the screen. */
static void kprint_at(const char *str, int row, int col)
{
    /* volatile: tell the compiler this memory can change outside the program
       (it's the screen), so it must not optimize the writes away. */
    volatile char *video = (volatile char *)VIDEO_MEMORY;
    int offset = 2 * (row * COLS + col);   /* byte offset of the (row,col) cell */

    for (int i = 0; str[i] != '\0'; i++) {
        video[offset + 2 * i]     = str[i];          /* the character */
        video[offset + 2 * i + 1] = GREEN_ON_BLACK;  /* its color */
    }
}

/* kmain is the entry point our assembly stub (kernel_entry.asm) calls.
   Returning from here is fine: the stub halts afterward. */
void kmain(void)
{
    kprint_at("PyroOS: C kernel is running. Hello from C.", 4, 0);
    kprint_at("We jumped from the boot sector into compiled C code.", 5, 0);
}
