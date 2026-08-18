# PyroOS

A custom x86 operating system built from the bootloader up, developed and debugged in QEMU.

The goal: an OS small enough to understand end to end. Every interrupt vector, every memory page, every scheduled task.

## Status

Milestone 10 complete: PyroOS is a small but real operating system. It boots from
a custom bootloader, runs a C kernel in 32-bit protected mode, has virtual memory
(paging) and a heap, handles interrupts, and runs timer and keyboard drivers. An
interactive shell accepts commands, a filesystem (PyroFS) stores files on disk
via a hand-written ATA driver, and a preemptive scheduler multitasks with real
context switching. Programs can run in ring 3 (user mode) and reach the kernel
only through int 0x80 system calls.

Shell commands: `help`, `about`, `clear`, `echo`, `ls`, `write`, `cat`, `mem`,
`ticks`, `disk`, `tasks`, `spin`, `syscall`, `user`, `fault`, `reboot`.

Run it: `make run` (or `make run-sdl` if the GTK window won't take keyboard
input under WSLg).

Layout: `boot/` holds the assembly boot sector and its pieces; `kernel/` holds
the C kernel and all subsystems (screen, interrupts, paging, heap, drivers,
filesystem, shell), the entry stub, and the linker script.

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
5. Memory: paging (virtual memory) and a kernel heap. (done)
6. Shell: an interactive command line. (done)
7. Storage: ATA disk driver and the PyroFS filesystem. (done)
8. Multitasking: a round-robin scheduler and context switching. (done)
9. Preemptive scheduling: the timer forcibly switches tasks. (done)
10. User mode: ring 3, a TSS, and system calls. (done)
11. Fault handling: exception reporting and page-fault recovery. (done)
