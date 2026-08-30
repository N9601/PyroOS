# ============================================================================
#  PyroOS build
#  Run from inside WSL (Ubuntu). Requires: nasm, gcc-multilib, binutils,
#  qemu-system-x86.
#
#  Pipeline:
#    boot/*.asm            --nasm-->  build/boot.bin      (flat 512-byte sector)
#    kernel/kernel_entry.asm --nasm--> build/kernel_entry.o (ELF object)
#    kernel/kernel.c       --gcc-->   build/kernel.o        (ELF object)
#    the two .o files       --ld-->   build/kernel.elf
#    kernel.elf         --objcopy-->  build/kernel.bin   (flat binary)
#    boot.bin + kernel.bin  --cat-->  build/os-image.bin (bootable disk image)
# ============================================================================

BUILD := build

# Freestanding 32-bit C: no OS, no standard library, no position-independent
# code or stack protector (those assume an OS runtime we don't have).
CC     := gcc
CFLAGS := -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra

# The boot sector depends on every file it %includes, so a change to any of
# them triggers a rebuild.
BOOT_SRCS := boot/boot.asm boot/gdt.asm boot/disk.asm boot/switch_pm.asm boot/print_pm.asm

# Every C file in kernel/ becomes an object automatically.
C_SOURCES := $(wildcard kernel/*.c)
C_OBJ     := $(patsubst kernel/%.c, $(BUILD)/%.o, $(C_SOURCES))

# Assembly objects that link with the C code. kernel_entry MUST come first in
# the link so its _start is the very first byte of the kernel.
ASM_OBJ := $(BUILD)/kernel_entry.o $(BUILD)/interrupt.o $(BUILD)/switch.o $(BUILD)/gdt_flush.o $(BUILD)/ring3.o

# Standalone user programs, compiled separately and embedded as byte arrays so
# the kernel can write them into the filesystem at boot.
EMBED_OBJ := $(BUILD)/userprog.o $(BUILD)/crashprog.o $(BUILD)/askprog.o \
             $(BUILD)/calcprog.o $(BUILD)/guessprog.o $(BUILD)/noteprog.o \
             $(BUILD)/progelf.o

.PHONY: all run clean

all: $(BUILD)/os-image.bin

# --- boot sector: flat binary, exactly 512 bytes ---
$(BUILD)/boot.bin: $(BOOT_SRCS)
	@mkdir -p $(BUILD)
	nasm -f bin -I boot/ boot/boot.asm -o $@

# --- kernel entry stub: ELF object so it can be linked with the C code ---
$(BUILD)/kernel_entry.o: kernel/kernel_entry.asm
	@mkdir -p $(BUILD)
	nasm -f elf32 $< -o $@

# --- interrupt stubs: ELF object ---
$(BUILD)/interrupt.o: kernel/interrupt.asm
	@mkdir -p $(BUILD)
	nasm -f elf32 $< -o $@

# --- context switch: ELF object ---
$(BUILD)/switch.o: kernel/switch.asm
	@mkdir -p $(BUILD)
	nasm -f elf32 $< -o $@

# --- GDT/TSS loader: ELF object ---
$(BUILD)/gdt_flush.o: kernel/gdt_flush.asm
	@mkdir -p $(BUILD)
	nasm -f elf32 $< -o $@

# --- user-mode entry / context save: ELF object ---
$(BUILD)/ring3.o: kernel/ring3.asm
	@mkdir -p $(BUILD)
	nasm -f elf32 $< -o $@

# --- generic rule: compile any kernel/*.c into build/*.o ---
$(BUILD)/%.o: kernel/%.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# --- standalone user program: compile, link at 0x80000, flatten ---
$(BUILD)/prog.o: user/prog.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/prog.bin: $(BUILD)/prog.o user/prog.ld
	ld -m elf_i386 -z noexecstack -T user/prog.ld -o $(BUILD)/prog.elf $(BUILD)/prog.o
	objcopy -O binary $(BUILD)/prog.elf $@

# --- embed the program as a C byte array (user_prog / user_prog_len) ---
$(BUILD)/userprog.c: $(BUILD)/prog.bin
	cd $(BUILD) && xxd -i prog.bin | \
		sed 's/prog_bin/user_prog/g; s/^unsigned char/const unsigned char/' > userprog.c

$(BUILD)/userprog.o: $(BUILD)/userprog.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- a second program (the misbehaving one), same pipeline ---
$(BUILD)/crash.o: user/crash.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/crash.bin: $(BUILD)/crash.o user/prog.ld
	ld -m elf_i386 -z noexecstack -T user/prog.ld -o $(BUILD)/crash.elf $(BUILD)/crash.o
	objcopy -O binary $(BUILD)/crash.elf $@

$(BUILD)/crashprog.c: $(BUILD)/crash.bin
	cd $(BUILD) && xxd -i crash.bin | \
		sed 's/crash_bin/crash_prog/g; s/^unsigned char/const unsigned char/' > crashprog.c

$(BUILD)/crashprog.o: $(BUILD)/crashprog.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- a third program (the interactive one), same pipeline ---
$(BUILD)/ask.o: user/ask.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/ask.bin: $(BUILD)/ask.o user/prog.ld
	ld -m elf_i386 -z noexecstack -T user/prog.ld -o $(BUILD)/ask.elf $(BUILD)/ask.o
	objcopy -O binary $(BUILD)/ask.elf $@

$(BUILD)/askprog.c: $(BUILD)/ask.bin
	cd $(BUILD) && xxd -i ask.bin | \
		sed 's/ask_bin/ask_prog/g; s/^unsigned char/const unsigned char/' > askprog.c

$(BUILD)/askprog.o: $(BUILD)/askprog.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- calculator and guessing game, same pipeline ---
$(BUILD)/calc.o: user/calc.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/calc.bin: $(BUILD)/calc.o user/prog.ld
	ld -m elf_i386 -z noexecstack -T user/prog.ld -o $(BUILD)/calc.elf $(BUILD)/calc.o
	objcopy -O binary $(BUILD)/calc.elf $@
$(BUILD)/calcprog.c: $(BUILD)/calc.bin
	cd $(BUILD) && xxd -i calc.bin | \
		sed 's/calc_bin/calc_prog/g; s/^unsigned char/const unsigned char/' > calcprog.c
$(BUILD)/calcprog.o: $(BUILD)/calcprog.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/guess.o: user/guess.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/guess.bin: $(BUILD)/guess.o user/prog.ld
	ld -m elf_i386 -z noexecstack -T user/prog.ld -o $(BUILD)/guess.elf $(BUILD)/guess.o
	objcopy -O binary $(BUILD)/guess.elf $@
$(BUILD)/guessprog.c: $(BUILD)/guess.bin
	cd $(BUILD) && xxd -i guess.bin | \
		sed 's/guess_bin/guess_prog/g; s/^unsigned char/const unsigned char/' > guessprog.c
$(BUILD)/guessprog.o: $(BUILD)/guessprog.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/note.o: user/note.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/note.bin: $(BUILD)/note.o user/prog.ld
	ld -m elf_i386 -z noexecstack -T user/prog.ld -o $(BUILD)/note.elf $(BUILD)/note.o
	objcopy -O binary $(BUILD)/note.elf $@
$(BUILD)/noteprog.c: $(BUILD)/note.bin
	cd $(BUILD) && xxd -i note.bin | \
		sed 's/note_bin/note_prog/g; s/^unsigned char/const unsigned char/' > noteprog.c
$(BUILD)/noteprog.o: $(BUILD)/noteprog.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- link kernel: entry stub FIRST, base address 0x10000, then flatten ---
$(BUILD)/kernel.bin: $(ASM_OBJ) $(C_OBJ) $(EMBED_OBJ) kernel/linker.ld
	ld -m elf_i386 -z noexecstack -T kernel/linker.ld -o $(BUILD)/kernel.elf \
		$(ASM_OBJ) $(C_OBJ) $(EMBED_OBJ)
	objcopy -O binary $(BUILD)/kernel.elf $@

# --- final disk image: boot sector then kernel, padded to 64 KB so the boot
#     loader's LBA read (KERNEL_SECTORS in boot.asm) always has data to read.
#     If the kernel grows past ~30 KB, raise KERNEL_SECTORS and this size. ---
$(BUILD)/os-image.bin: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $@
	truncate -s 1048576 $@
	@echo "Built $@ (kernel: $$(stat -c%s $(BUILD)/kernel.bin) bytes)"

run: all
	qemu-system-i386 -drive format=raw,file=$(BUILD)/os-image.bin -display gtk

# Alternative display: under WSLg the SDL backend often forwards keyboard
# input more reliably than GTK. Try this if you can see PyroOS but can't type.
run-sdl: all
	qemu-system-i386 -drive format=raw,file=$(BUILD)/os-image.bin -display sdl

clean:
	rm -rf $(BUILD)

# --- embed the linked ELF of prog, not just its flattened bytes ---
# The flat .bin threw away the entry point, the segment table and the .bss size.
# Keeping the ELF gives the loader all three. Stripped first, because symbol and
# string tables are for debuggers and would only pad the kernel image.
$(BUILD)/prog_stripped.elf: $(BUILD)/prog.bin
	objcopy --strip-all $(BUILD)/prog.elf $@

$(BUILD)/progelf.c: $(BUILD)/prog_stripped.elf
	cd $(BUILD) && xxd -i prog_stripped.elf | \
		sed 's/prog_stripped_elf/prog_elf/g; s/^unsigned char/const unsigned char/' > progelf.c

$(BUILD)/progelf.o: $(BUILD)/progelf.c
	$(CC) $(CFLAGS) -c $< -o $@
