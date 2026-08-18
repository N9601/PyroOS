/* ============================================================================
 *  PyroOS  -  kernel linker script
 * ----------------------------------------------------------------------------
 *  A linker takes our compiled pieces (kernel_entry.o, kernel.o) and stitches
 *  them into one image. This script controls that layout.
 *
 *  Two critical decisions:
 *    1. ". = 0x1000" sets the starting address. Every absolute address the
 *       compiler bakes in (string locations, function calls) is computed as
 *       if the kernel lives at 0x1000 -- which is exactly where the boot
 *       sector loads it. If these disagreed, the kernel would jump to wrong
 *       addresses and crash.
 *    2. We list kernel_entry's code first inside .text so the _start stub is
 *       the very first byte, letting the boot sector jump straight to it.
 *       (We also pass kernel_entry.o first on the linker command line, which
 *       reinforces the ordering.)
 * ==========================================================================*/

ENTRY(_start)

SECTIONS
{
    . = 0x10000;                /* kernel is loaded and linked at 0x10000 (64 KB) */

    .text : {
        *(.text)                /* all code; entry stub comes first by link order */
    }

    .rodata : {
        *(.rodata*)             /* read-only data: our string literals live here */
    }

    .data : {
        *(.data)                /* initialized global/static data */
    }

    .bss : {
        bss_start = .;          /* entry stub zeroes the range [bss_start,bss_end) */
        *(.bss)
        *(COMMON)               /* zero-initialized data */
        bss_end = .;
    }
}
