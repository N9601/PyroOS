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

# --- link kernel: entry stub FIRST, base address 0x1000, then flatten ---
$(BUILD)/kernel.bin: $(ASM_OBJ) $(C_OBJ) kernel/linker.ld
	ld -m elf_i386 -z noexecstack -T kernel/linker.ld -o $(BUILD)/kernel.elf \
		$(ASM_OBJ) $(C_OBJ)
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
