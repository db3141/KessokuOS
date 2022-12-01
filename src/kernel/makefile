CXX=i686-elf-g++

PWD=$(shell pwd)

CXXFLAGS=-ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -ggdb -Wall -Wextra -mgeneral-regs-only -I$(PWD)
LINK_FLAGS=-lgcc

BUILD_OUT=$(BUILD_DIR)/kernel

OUTPUT_FILE=$(BUILD_OUT)/kernel.elf
UNSTRIPPED_OUTPUT_FILE=$(BUILD_OUT)/kernel_unstripped.elf
LINKER_SCRIPT=kernel.lds

SOURCE_FILES=\
	kernel.cpp\
	common.cpp\
	gdt.cpp\
	\
	drivers/dma/dma.cpp\
	drivers/vga/vga.cpp\
	drivers/ps2/ps2.cpp\
	drivers/ps2/keyboard/keyboard.cpp\
	drivers/pit/pit.cpp\
	drivers/disk/floppy/floppy.cpp\
	\
	interrupts/idt.cpp\
	interrupts/pic.cpp\
	interrupts/interrupt_handler.cpp\
	\
	memory-manager/manager.cpp\

HEADER_FILES=\
	common.hpp\
	error.hpp\
	gdt.hpp\
	\
	drivers/dma/dma.hpp\
	drivers/vga/vga.hpp\
	drivers/ps2/ps2.hpp\
	drivers/ps2/keyboard/keyboard.hpp\
	drivers/pit/pit.hpp\
	drivers/disk/floppy/floppy_disk.hpp\
	\
	interrutps/idt.hpp\
	interrupts/interrupt_handler.hpp\
	interrupts/pic.hpp\
	\
	memory-manager/manager.hpp\
	\
	data/error_or.hpp\
	data/queue.hpp\
	data/fc_vector.hpp\


OBJS=$(patsubst %.cpp,$(BUILD_OUT)/%.o,$(SOURCE_FILES))

CRTI_OBJ:=$(BUILD_OUT)/crt/crti.o
CRTBEGIN_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)
CRTN_OBJ:=$(BUILD_OUT)/crt/crtn.o

OBJ_LINK_LIST:=$(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

BUILD_OUT_DIRECTORIES=$(abspath $(dir $(OBJS) $(CRTI_OBJ) $(CRTN_OBJ))) # get the objects' canonical paths
BUILD_OUT_DIRECTORIES_UNIQUE=$(shell echo $(BUILD_OUT_DIRECTORIES) | sed -e 's/ /\n/g' | awk -F "/" '{print NF-1" "$$0}' | sort -n | cut -f 2- -d ' ' | uniq) # sort by directory depth and remove duplicates

all: $(OUTPUT_FILE)

$(BUILD_OUT)/crt/%.o: crt/%.asm | create_build_dir
	nasm -f elf $< -o $@ -w+all

$(BUILD_OUT)/%.o: %.cpp | create_build_dir
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LINK_FLAGS)

$(OUTPUT_FILE): $(UNSTRIPPED_OUTPUT_FILE)
	strip -s $(UNSTRIPPED_OUTPUT_FILE) -o $(OUTPUT_FILE)

$(UNSTRIPPED_OUTPUT_FILE): $(OBJ_LINK_LIST)
	$(CXX) -T $(LINKER_SCRIPT) -o $(UNSTRIPPED_OUTPUT_FILE) $(OBJ_LINK_LIST) $(CXXFLAGS) $(LINK_FLAGS)

.PHONY: create_build_dir
create_build_dir:
	-mkdir $(BUILD_OUT)
	-mkdir -p $(BUILD_OUT_DIRECTORIES_UNIQUE)

.PHONY: clean
clean:
	-rm -rf $(BUILD_OUT)
