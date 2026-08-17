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

# --- C kernel: ELF object ---
$(BUILD)/kernel.o: kernel/kernel.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# --- link kernel: entry stub FIRST, base address 0x1000, then flatten ---
$(BUILD)/kernel.bin: $(BUILD)/kernel_entry.o $(BUILD)/kernel.o kernel/linker.ld
	ld -m elf_i386 -z noexecstack -T kernel/linker.ld -o $(BUILD)/kernel.elf \
		$(BUILD)/kernel_entry.o $(BUILD)/kernel.o
	objcopy -O binary $(BUILD)/kernel.elf $@

# --- final disk image: boot sector then kernel, padded so the boot sector's
#     15-sector read never runs past the end of the image (16 sectors here).
#     If the kernel ever grows past ~7.5 KB, raise both this size and the
#     sector count in boot.asm. ---
$(BUILD)/os-image.bin: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin > $@
	truncate -s 8192 $@
	@echo "Built $@ (kernel: $$(stat -c%s $(BUILD)/kernel.bin) bytes)"

run: all
	qemu-system-i386 -drive format=raw,file=$(BUILD)/os-image.bin

clean:
	rm -rf $(BUILD)
