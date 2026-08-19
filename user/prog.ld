/* ============================================================================
 *  PyroOS  -  linker script for standalone user programs
 * ----------------------------------------------------------------------------
 *  Programs run at 0x80000 (512 KB): above the kernel and below the kernel
 *  stack, inside the identity-mapped, user-accessible first 4 MB. The _start
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
    .data   : { *(.data)   *(.data.*) }
    .bss    : { *(.bss) *(.bss.*) *(COMMON) }
}
