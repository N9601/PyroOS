# PyroOS

A custom x86 operating system built from the bootloader up, developed and debugged in QEMU.

The goal: an OS small enough to understand end to end. Every interrupt vector, every memory page, every scheduled task.

## Status

Milestone 4 complete: PyroOS is interactive. On top of the boot sector,
protected-mode switch, and C kernel, it now installs an interrupt descriptor
table, remaps the PIC, and runs timer (IRQ0) and PS/2 keyboard (IRQ1) drivers.
Typed characters echo to the screen through a VGA text driver with a scrolling
cursor.

Layout: `boot/` holds the assembly boot sector and its pieces; `kernel/` holds
the C kernel, the screen and interrupt code, the keyboard and timer drivers, the
entry stub, and the linker script.

## Toolchain

Built inside WSL2 (Ubuntu 24.04):

- NASM (assembler)
- QEMU (`qemu-system-i386`, emulator)
- GCC with 32-bit multilib, used in freestanding mode (`-m32 -ffreestanding
  -nostdlib`) to compile the C kernel. A dedicated `i686-elf` cross-compiler is
  an optional future upgrade; the freestanding host compiler produces the same
  bare-metal 32-bit code.

## Build and run

From inside WSL, in the project root:

```
make        # assemble build/boot.bin
make run    # boot it in QEMU
```

## Roadmap

1. Boot sector: BIOS handoff, real mode, print via BIOS interrupts. (done)
2. Protected mode: GDT, A20 line, far jump to 32-bit. (done)
3. C kernel: linker script, handoff from assembly to C, VGA text driver. (done)
4. Interrupts: IDT, PIC, keyboard and timer handlers. (done)
5. Beyond: paging, memory management, a scheduler. (next)
