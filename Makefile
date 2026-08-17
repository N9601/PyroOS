# ============================================================================
#  PyroOS build
#  Run from inside WSL (Ubuntu). Requires: nasm, qemu-system-x86.
# ============================================================================

BUILD := build

.PHONY: all run clean

all: $(BUILD)/boot.bin

# Assemble the boot sector as a flat binary (-f bin): no ELF headers, no
# metadata, just the raw 512 bytes the BIOS expects.
$(BUILD)/boot.bin: boot/boot.asm
	@mkdir -p $(BUILD)
	nasm -f bin $< -o $@
	@echo "Built $@ ($$(stat -c%s $@) bytes)"

# Boot it in QEMU. -drive ...format=raw tells QEMU this file is a raw disk
# image, not a qcow2 etc. The BIOS will load sector 0 and run it.
run: all
	qemu-system-i386 -drive format=raw,file=$(BUILD)/boot.bin

clean:
	rm -rf $(BUILD)
