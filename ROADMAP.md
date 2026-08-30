# PyroOS Roadmap

PyroOS is a from-scratch x86 operating system. This roadmap tracks what is done
and the large subsystems planned next.

## Done (Milestones 1 to 18)

Bootloader and BIOS handoff, 16-bit real mode to 32-bit protected mode (GDT, A20),
a freestanding C kernel, interrupts (IDT, 8259 PIC, keyboard and timer drivers),
virtual memory (paging) and a kernel heap, an interactive shell with command
history, an ATA (IDE) PIO disk driver and the PyroFS filesystem, cooperative and
preemptive scheduling with context switching, ring-3 user mode with a Task State
Segment and int 0x80 system calls, user/supervisor memory protection, a program
loader that runs separately-compiled ELF-style programs from disk, and userland
apps (a calculator, a guessing game, and a text editor that saves files).

## Planned (the big subsystems)

### M19. Preemptive kernel threads and synchronization primitives
Preemptive, priority-based scheduling with kernel threads, timer-driven context
switching, and synchronization primitives: spinlocks, mutexes, and counting
semaphores. Demonstrates race conditions and mutual exclusion.
Keywords: preemptive scheduling, multithreading, concurrency, context switching,
mutex, semaphore, spinlock, atomics, race conditions, critical sections.

### M20. Per-process virtual address spaces
Per-process page directories with a page-frame allocator, demand paging, and
copy-on-write, isolating processes through the MMU.
Keywords: virtual memory, MMU, paging, demand paging, copy-on-write, page fault
handler, memory isolation, TLB.

### M21. POSIX-style process model (part 1 done)
A UNIX-style process model with fork, exec, and wait, a full ELF binary loader,
argument passing, and process lifecycle management.
Keywords: POSIX, process management, fork, exec, ELF loader, system calls, ABI,
inter-process communication.

Done so far: the process table, address-space cloning, and the fork/exit/wait
lifecycle with zombie reaping and orphan re-parenting. Still to come: exec with
a real ELF loader replacing the flat-binary loader, argument passing, exposing
the three calls to ring 3 through int 0x80, and copy-on-write instead of the
current eager page copy.

### M22. Virtual File System and a real on-disk filesystem
A Virtual File System layer over a FAT32 or ext2 driver, with a block buffer
cache, mount points, and a POSIX file API (open, read, write, seek).
Keywords: file system, VFS, FAT32, ext2, inodes, block cache, mounting, POSIX I/O.

### M23. PCI enumeration and a device-driver framework
A PCI bus enumeration layer and a device-driver framework, with drivers for
AHCI/SATA storage, the real-time clock, and 16550 serial.
Keywords: device drivers, PCI, DMA, AHCI, SATA, hardware abstraction layer,
interrupt handling, memory-mapped I/O.

### M24. TCP/IP network stack
A TCP/IP stack from scratch (Ethernet, ARP, IPv4, ICMP, UDP, TCP) with an
RTL8139 or e1000 NIC driver, DHCP, and a BSD-style socket API.
Keywords: TCP/IP, networking, network stack, sockets, Ethernet, ARP, IPv4, UDP,
TCP, DHCP, NIC driver, packet processing.

### M25. Graphical subsystem
A VESA/GOP framebuffer driver, a double-buffered compositor, mouse input, and a
windowing GUI with widgets.
Keywords: graphics, framebuffer, GUI, compositor, window manager, double
buffering, event loop, rendering.

### M26. C standard library and userland
A freestanding C standard library, userland coreutils, and a shell with pipes,
I/O redirection, and job control.
Keywords: libc, userland, POSIX, coreutils, toolchain, pipes, redirection.

### M27. Symmetric multiprocessing (SMP)
Multi-core support: APIC-based CPU bring-up, per-CPU run queues, and locked and
lock-free data structures for cache-coherent scheduling.
Keywords: SMP, multi-core, parallelism, APIC, per-CPU, cache coherency, atomics,
lock-free, scalability.

### M28. 64-bit port and security hardening
A port to x86-64 long mode with a higher-half address layout, 4-level paging, and
security hardening (W^X, ASLR, SMEP and SMAP).
Keywords: x86-64, long mode, 64-bit, higher-half kernel, ASLR, W^X, SMEP, SMAP,
exploit mitigation.
