export BUILD_DIR:=$(shell pwd)/build

BOOTLOADER_DIR:=src/bootloader
KERNEL_DIR:=src/kernel

BOOT0=$(BUILD_DIR)/bootloader/boot0.o
BOOT1=$(BUILD_DIR)/bootloader/boot1.o
OS=$(BUILD_DIR)/kernel/kernel.elf

ARCHIVE_FILES=$(BOOT1) $(OS) media/*
FILE_ARCHIVE=$(BUILD_DIR)/file_system.tar

DISK_IMG=$(BUILD_DIR)/bootdisk.img

all: bootdisk

.PHONY: bootdisk bootloader kernel

bootloader: | create_build_dir
	make -C $(BOOTLOADER_DIR)

kernel: | create_build_dir
	make -C $(KERNEL_DIR)

bootdisk: bootloader kernel
	dd if=/dev/zero of=$(DISK_IMG) bs=512 count=2880
	dd conv=notrunc if=$(BOOT0) of=$(DISK_IMG) bs=512 count=1 seek=0
	tar -c --format=ustar --transform='s,.*/,,gsr' -f $(FILE_ARCHIVE) $(ARCHIVE_FILES)
	dd conv=notrunc if=$(FILE_ARCHIVE) of=$(DISK_IMG) bs=512 seek=1

.PHONY: clean
clean:
	-make -C $(BOOTLOADER_DIR) clean
	-make -C $(KERNEL_DIR) clean
	-rm $(BUILD_DIR)/*
	-rmdir $(BUILD_DIR)

.PHONY: create_build_dir
create_build_dir: | clean
	-mkdir $(BUILD_DIR)
