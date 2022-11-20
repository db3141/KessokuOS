BUILD_OUT:=$(BUILD_DIR)/bootloader

SOURCE_FILES:=bootloader.asm

BOOT0=$(BUILD_OUT)/boot0.o
BOOT1=$(BUILD_OUT)/boot1.o
BOOTLOADER=$(BUILD_OUT)/bootloader.o

OBJS=$(BOOT0) $(BOOT1) $(BOOTLOADER)

all: $(OBJS)

$(BOOTLOADER): $(SOURCE_FILES) | create_build_dir
	nasm -f elf $< -F dwarf -g -o $@ -w+all
	i686-elf-ld -T $(patsubst $(BUILD_OUT)/%.o,%.lds,$@) $@ -o $@.elf
	objcopy -O binary $@.elf $@

$(BOOT0): $(BUILD_OUT)/bootloader.o
	dd if=$(BOOTLOADER) of=$(BOOT0) bs=512 count=1 seek=0

$(BOOT1): $(BUILD_OUT)/bootloader.o
	dd if=$(BOOTLOADER) of=$(BOOT1) bs=512 skip=1

.PHONY: clean
clean:
	-rm $(BUILD_OUT)/*
	-rmdir $(BUILD_OUT)

.PHONY: create_build_dir
create_build_dir:
	-mkdir $(BUILD_OUT)

