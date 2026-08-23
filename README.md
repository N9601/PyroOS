<div align="center">

<img src="./assets/logo.svg" alt="PyroOS" width="150" />

# PyroOS

![Milestones](https://img.shields.io/badge/Milestones-18-e07a25?style=flat-square)
![Language](https://img.shields.io/badge/C-freestanding-00599C?style=flat-square&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/Assembly-NASM-6E4C13?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-x86%2032--bit-5b6b8a?style=flat-square)
![Emulator](https://img.shields.io/badge/QEMU-i386-FF6600?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-16a34a?style=flat-square)
![Status](https://img.shields.io/badge/Status-WIP-blueviolet?style=flat-square)

### **A from-scratch x86 operating system.**

Bootloader, 32-bit protected mode, a C kernel, paging, a heap, interrupts, a filesystem, preemptive multitasking, ring-3 user mode, and system calls. No libraries, no host OS underneath. Every byte that runs is code in this repo.

<sub>Bare metal. From the bootloader up.</sub>

[Quickstart](#quickstart) | [Why](#why) | [Features](#features) | [Shell](#shell) | [Architecture](#architecture) | [Build](#build) | [Layout](#project-layout) | [Roadmap](#roadmap)

</div>

---

## Quickstart

PyroOS is developed and run inside [QEMU](https://www.qemu.org/), so you never touch real hardware. On Windows the toolchain lives in WSL2 (Ubuntu).

**1. Install the toolchain** (inside WSL/Ubuntu):

```bash
sudo apt install nasm qemu-system-x86 build-essential gcc-multilib
```

**2. Build and boot it:**

```bash
git clone https://github.com/N9601/PyroOS.git
cd PyroOS
make run
```

A QEMU window opens, shows the fire logo, and lands you at the `pyro>` shell. If the window will not take keyboard input under WSLg, use `make run-sdl` and click inside the window.

**3. Drive it:**

```
pyro> help
pyro> logo
pyro> write hello world
pyro> cat hello
pyro> ls
pyro> exec ask
pyro> exec calc
pyro> exec guess
pyro> exec crash
pyro> tasks
pyro> spin
pyro> user
pyro> fault
```

---

## Why

Most people never see below their programming language. PyroOS is the opposite: it starts at the first instruction the CPU runs after power-on and builds up, one layer at a time, until there is a shell you can type commands at. The goal is an OS small enough to understand end to end. Every interrupt vector, every memory page, every scheduled task is code you can read here.

It is not a Linux distribution and does not try to be. It is a real operating-system kernel: it boots, protects memory, multitasks, stores files, and separates user code from the kernel. That is the core of what an OS is.

---

## Features

<table>
<tr>
<td width="50%" valign="top">

### Boot and CPU
- 512-byte boot sector, BIOS handoff at `0x7C00`
- Real mode to 32-bit protected mode: GDT, A20 line, far jump
- LBA disk loading via BIOS `INT 13h` extended read
- Kernel relocated to `0x10000` for headroom

### Memory
- Paging: identity-mapped first 4 MB, `CR0.PG` enabled
- Kernel heap: first-fit `kmalloc` / `kfree` with split and coalesce
- `.bss` zeroed at startup by the entry stub

### Interrupts
- IDT with 32 CPU-exception and 16 hardware-IRQ handlers
- 8259 PIC remapped to vectors 32 to 47
- Page-fault reporting with `CR2`, plus recoverable faults

</td>
<td width="50%" valign="top">

### Concurrency
- Cooperative round-robin scheduler
- Preemptive scheduling driven by the timer (IRQ0)
- Context switching hand-written in assembly

### Storage
- ATA (IDE) PIO disk driver, 28-bit LBA
- PyroFS: superblock, directory, and file data on disk
- `ls`, `write`, and `cat` from the shell

### Userland
- Ring-3 user mode via a Task State Segment
- `int 0x80` syscalls: write, read, uptime, sleep, rand, fwrite, fread, exit
- Loads and runs separately-compiled programs from disk
- Apps: a calculator, a guessing game, and a note editor that saves files
- User/supervisor memory protection, with syscall pointer validation

### Interface
- Interactive shell with command history (up and down arrows)
- Keyboard with Shift and arrow-key support
- VGA text driver with a cursor and scrolling
- Pixel-art flame logo as a boot splash

</td>
</tr>
</table>

---

## Shell

The shell reads a line, parses it, and runs a built-in command.

```
help          list commands              tasks     cooperative multitasking demo
about         what PyroOS is             spin      preemptive multitasking demo
clear         clear the screen           syscall   invoke a system call (int 0x80)
logo          show the flame logo        user      run the built-in ring-3 demo
echo <text>   print text back            exec <f>  load and run a program (ring 3)
ls            list files                 fault     trigger and recover a page fault
write <f> <t> write text to a file       disk      check the boot disk
cat <f>       print a file               ticks     timer ticks since boot
mem           test the heap allocator    reboot    restart the machine
```

---

## Architecture

From power-on to the shell, each layer built on the one below it.

```
   POWER ON
      |
      v
   [ BIOS ]  --reads 512 bytes-->  [ boot sector @ 0x7C00 ]      16-bit real mode
                                          |  print via BIOS, load kernel by LBA
                                          v
                                   [ switch to protected mode ]  GDT, A20, far jump
                                          |
                                          v
                                   [ 32-bit C kernel @ 0x10000 ]
                                          |
        +---------------+-----------------+-----------------+----------------+
        |               |                 |                 |                |
   +----v----+   +------v-----+   +-------v------+   +------v------+   +-----v-----+
   | paging  |   |   heap     |   |  interrupts  |   |  ATA + FS   |   | scheduler |
   | CR0.PG  |   | kmalloc    |   | IDT/PIC/IRQ  |   |  PyroFS     |   | preempt   |
   +----+----+   +------+-----+   +-------+------+   +------+------+   +-----+-----+
        +---------------+-----------------+-----------------+----------------+
                                          |
                        +-----------------v------------------+
                        |  shell (ring 0)                    |
                        |  drops to ring-3 user mode via TSS |
                        |  syscalls trap back via int 0x80   |
                        +------------------------------------+
```

Memory map: boot sector at `0x7C00`, kernel at `0x10000`, protected-mode stack at `0x90000`, kernel heap from `0x100000` to `0x400000`, VGA text memory at `0xB8000`.

---

## Build

Everything runs inside WSL2 (Ubuntu). Requires `nasm`, `qemu-system-x86`, `build-essential`, and `gcc-multilib`.

```bash
make          # assemble the boot sector, compile the kernel, link the disk image
make run      # boot build/os-image.bin in QEMU (GTK window)
make run-sdl  # same, SDL window (better keyboard forwarding under WSLg)
make clean    # remove build artifacts
```

The build pipeline: the boot sector assembles to a flat 512-byte binary; the C kernel and its assembly stubs compile to ELF objects, link at base `0x10000`, and are flattened with `objcopy`; the two are concatenated into a bootable disk image.

The C kernel is compiled freestanding (`-m32 -ffreestanding -nostdlib -fno-pie`), so it makes no assumptions about a host operating system.

---

## Project layout

```
PyroOS/
├── boot/
│   ├── boot.asm         Stage 1: real mode, load kernel, switch to 32-bit
│   ├── gdt.asm          Global Descriptor Table (flat segments)
│   ├── disk.asm         LBA disk load via INT 13h AH=42h
│   ├── switch_pm.asm     Real mode to protected mode transition
│   └── print_pm.asm      Protected-mode VGA printing
├── kernel/
│   ├── kernel.c         kmain: brings up every subsystem
│   ├── kernel_entry.asm  32-bit entry stub, zeroes .bss, calls kmain
│   ├── linker.ld        Links the kernel at 0x10000
│   ├── screen.c         VGA text driver (cursor, scroll, colors)
│   ├── idt.c            Interrupt Descriptor Table
│   ├── interrupt.asm     ISR/IRQ/syscall stubs
│   ├── isr.c            Interrupt dispatch, PIC remap, fault handling
│   ├── keyboard.c       PS/2 keyboard (IRQ1) with an input ring buffer
│   ├── timer.c          Programmable interval timer (IRQ0)
│   ├── paging.c         Virtual memory
│   ├── kheap.c          Kernel heap (kmalloc / kfree)
│   ├── ata.c            ATA PIO disk driver
│   ├── fs.c             PyroFS filesystem
│   ├── task.c           Scheduler (cooperative and preemptive)
│   ├── switch.asm        Context switch
│   ├── gdt.c            Kernel GDT with user segments and a TSS
│   ├── syscall.c        int 0x80 system-call dispatch
│   ├── usermode.c       Ring-3 launcher (with ring3.asm)
│   ├── logo.c           Pixel-art flame logo
│   ├── shell.c          The interactive shell
│   └── string.c         Freestanding memcpy, memset, strcmp, ...
├── user/
│   ├── prog.c           A well-behaved standalone program
│   ├── crash.c          A program that tries to corrupt the kernel (gets killed)
│   ├── ask.c            An interactive program (reads your name, greets you)
│   ├── calc.c           A calculator
│   ├── guess.c          A number-guessing game
│   ├── note.c           A text editor that saves files to PyroFS
│   └── prog.ld          Links programs to run at 0x80000
└── Makefile
```

---

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
12. A pixel-art flame logo (boot splash and `logo` command). (done)
13. Program loader: run separately-compiled programs from the filesystem in ring 3. (done)
14. Memory protection: user/supervisor pages; a bad program is killed, not the kernel. (done)
15. Interactive userland: blocking input syscalls; a ring-3 program reads and responds. (done)
16. Apps: a calculator and a guessing game, plus keyboard Shift support. (done)
17. File syscalls: a ring-3 note editor saves and reloads files from PyroFS. (done)
18. Shell command history and arrow-key support in the keyboard driver. (done)

Next: per-process address spaces (separate page tables per program), a VESA framebuffer for graphics, and a fuller ELF loader.

---

## License

MIT. See [LICENSE](./LICENSE).

---

<div align="center">

**18 milestones.** From a 512-byte boot sector to ring-3 apps, saved files, and a shell with history.
Built from scratch in x86 assembly and freestanding C.

</div>
