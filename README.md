# PyroOS

A custom x86 operating system built from the bootloader up, developed and debugged in QEMU.

The goal: an OS small enough to understand end to end. Every interrupt vector, every memory page, every scheduled task.

## Status

Milestone 1: stage 1 boot sector. Prints a banner in 16-bit real mode, then halts.

## Toolchain

Built inside WSL2 (Ubuntu 24.04):

- NASM (assembler)
- QEMU (`qemu-system-i386`, emulator)
- GCC / an `i686-elf` cross-compiler (added at Milestone 3, for the C kernel)

## Build and run

From inside WSL, in the project root:

```
make        # assemble build/boot.bin
make run    # boot it in QEMU
```

## Roadmap

1. Boot sector: BIOS handoff, real mode, print via BIOS interrupts. (done)
2. Protected mode: GDT, A20 line, far jump to 32-bit.
3. C kernel: linker script, handoff from assembly to C, VGA text driver.
4. Interrupts: IDT, PIC, keyboard and timer handlers.
5. Beyond: paging, memory management, a scheduler.
