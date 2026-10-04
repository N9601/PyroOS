/* ============================================================================
 *  PyroOS  -  linker script for standalone user programs
 * ----------------------------------------------------------------------------
 *  Programs run at 0x80000 (512 KB): above the kernel, at the bottom of the
 *  identity-mapped user zone (0x80000 to 0xFFFFF), the only memory ring 3 is
 *  allowed to touch. The _start
 *  stub (.text.start) is placed first so the flat binary's first byte is the
 *  entry point.
 * ==========================================================================*/
ENTRY(_start)

SECTIONS
{
    . = 0x80000;

    .text : {
        *(.text.start)
        *(.text)
        *(.text.*)
    }
    .rodata : { *(.rodata) *(.rodata.*) }

    /* Writable data starts on a page of its own. Page permissions are per
       page, so if .data or .bss shared a page with the code, the linker would
       have to merge them into one read-write-execute segment and the loader
       could not make the code read-only. */
    . = ALIGN(0x1000);
    .data   : { *(.data)   *(.data.*) }
    .bss    : { *(.bss) *(.bss.*) *(COMMON) }
}
