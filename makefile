BUILD_DIR:=build

BOOT0=$(BUILD_DIR)/bootloader/boot0.o
BOOT1=$(BUILD_DIR)/bootloader/boot1.o
OS=$(BUILD_DIR)/os/os.elf

ARCHIVE_FILES=$(BOOT1) $(OS)
FILE_ARCHIVE=$(BUILD_DIR)/file_system.tar

DISK_IMG=$(BUILD_DIR)/bootdisk.img

all: bootdisk

.PHONY: bootdisk bootloader os

bootloader: | create_build_dir
	make -C bootloader

os: | create_build_dir
	make -C os

bootdisk: bootloader os
	dd if=/dev/zero of=$(DISK_IMG) bs=512 count=2880
	dd conv=notrunc if=$(BOOT0) of=$(DISK_IMG) bs=512 count=1 seek=0
	tar -c --format=ustar --transform='s,.*/,,gsr' -f $(FILE_ARCHIVE) $(ARCHIVE_FILES)
	dd conv=notrunc if=$(FILE_ARCHIVE) of=$(DISK_IMG) bs=512 seek=1

.PHONY: clean
clean:
	-make -C bootloader clean
	-make -C os clean
	-rm $(BUILD_DIR)/*
	-rmdir $(BUILD_DIR)

.PHONY: create_build_dir
create_build_dir: | clean
	-mkdir $(BUILD_DIR)
