/* ============================================================================
 *  PyroOS  -  kernel linker script
 * ----------------------------------------------------------------------------
 *  A linker takes our compiled pieces (kernel_entry.o, kernel.o) and stitches
 *  them into one image. This script controls that layout.
 *
 *  Two critical decisions:
 *    1. ". = 0x10000" sets the starting address. Every absolute address the
 *       compiler bakes in (string locations, function calls) is computed as
 *       if the kernel lives at 0x10000 -- which is exactly where the boot
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
    image_end = .;              /* objcopy's flat binary stops here */

    .bss : {
        bss_start = .;          /* entry stub zeroes the range [bss_start,bss_end) */
        *(.bss)
        *(COMMON)               /* zero-initialized data */
        bss_end = .;
    }

    /* gcc emits .eh_frame unwind tables by default. They are only read by a
       C++ exception unwinder or a debugger walking the stack, and the kernel
       has neither, so keeping them would spend about 5 KB of the boot
       loader's KERNEL_SECTORS budget on bytes nothing ever reads. */
    /DISCARD/ : {
        *(.eh_frame)
    }
}

/* The boot sector loads exactly KERNEL_SECTORS (boot/boot.asm) sectors. A
   bigger kernel is not an error at boot, it is silently cut short, which shows
   up much later as missing code or corrupt data. Refuse to link one instead.
   120 must match KERNEL_SECTORS. */
ASSERT(image_end - 0x10000 <= 120 * 512,
       "kernel image exceeds KERNEL_SECTORS; raise it in boot/boot.asm and here")

/* Everything the kernel owns, .bss included, must end below the user zone at
   0x80000, which ring 3 can read and write. */
ASSERT(bss_end <= 0x80000, "kernel .bss runs into the user zone at 0x80000")
