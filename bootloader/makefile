BUILD_DIR:=../build/bootloader

SOURCE_FILES:=bootloader.asm

BOOT0=$(BUILD_DIR)/boot0.o
BOOT1=$(BUILD_DIR)/boot1.o
BOOTLOADER=$(BUILD_DIR)/bootloader.o

OBJS=$(BOOT0) $(BOOT1) $(BOOTLOADER)

all: $(OBJS)

$(BOOTLOADER): $(SOURCE_FILES) | create_build_dir
	nasm -f elf $< -F dwarf -g -o $@ -w+all
	i686-elf-ld -T $(patsubst $(BUILD_DIR)/%.o,%.lds,$@) $@ -o $@.elf
	objcopy -O binary $@.elf $@

$(BOOT0): $(BUILD_DIR)/bootloader.o
	dd if=$(BOOTLOADER) of=$(BOOT0) bs=512 count=1 seek=0

$(BOOT1): $(BUILD_DIR)/bootloader.o
	dd if=$(BOOTLOADER) of=$(BOOT1) bs=512 skip=1

.PHONY: clean
clean:
	-rm $(BUILD_DIR)/*
	-rmdir $(BUILD_DIR)

.PHONY: create_build_dir
create_build_dir:
	-mkdir $(BUILD_DIR)

