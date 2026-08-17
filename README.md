# PyroOS

A custom x86 operating system built from the bootloader up, developed and debugged in QEMU.

The goal: an OS small enough to understand end to end. Every interrupt vector, every memory page, every scheduled task.

## Status

Milestone 3 complete: the boot sector loads a kernel from disk, switches the CPU
to 32-bit protected mode, and jumps into a kernel written in C. The C kernel
prints to the screen by writing directly to VGA memory.

Layout: `boot/` holds the assembly boot sector and its pieces; `kernel/` holds
the C kernel, its entry stub, and the linker script.

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
4. Interrupts: IDT, PIC, keyboard and timer handlers. (next)
5. Beyond: paging, memory management, a scheduler.
