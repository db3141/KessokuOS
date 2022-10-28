CXX=i686-elf-g++

PWD=$(shell pwd)

CXXFLAGS=-ffreestanding -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -ggdb -Wall -Wextra -mgeneral-regs-only -I$(PWD)
LINK_FLAGS=-lgcc

BUILD_DIR=../build/os

OUTPUT_FILE=$(BUILD_DIR)/os.elf
UNSTRIPPED_OUTPUT_FILE=$(BUILD_DIR)/os_unstripped.elf
LINKER_SCRIPT=os.lds

SOURCE_FILES=\
	os.cpp\
	common.cpp\
	gdt.cpp\
	\
	drivers/dma.cpp\
	drivers/vga.cpp\
	drivers/ps2.cpp\
	drivers/ps2_keyboard.cpp\
	drivers/pit.cpp\
	drivers/floppy_disk.cpp\
	\
	interrupts/idt.cpp\
	interrupts/pic.cpp\
	interrupts/interrupt_handler.cpp\
	\
	memory-manager/manager.cpp\

HEADER_FILES=\
	common.hpp\
	gdt.hpp\
	\
	drivers/dma.hpp\
	drivers/vga.hpp\
	drivers/ps2.hpp\
	drivers/ps2_keyboard.hpp\
	drivers/pit.hpp\
	drivers/floppy_disk.hpp\
	\
	interrutps/idt.hpp\
	interrupts/interrupt_handler.hpp\
	interrupts/pic.hpp\
	\
	memory-manager/manager.hpp\
	\
	data/queue.hpp\
	data/fc_vector.hpp\


OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCE_FILES))

CRTI_OBJ:=$(BUILD_DIR)/crt/crti.o
CRTBEGIN_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ:=$(shell $(CXX) $(CXXFLAGS) -print-file-name=crtend.o)
CRTN_OBJ:=$(BUILD_DIR)/crt/crtn.o

OBJ_LINK_LIST:=$(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

all: $(OUTPUT_FILE)

$(BUILD_DIR)/crt/%.o: crt/%.asm | create_build_dir
	nasm -f elf $< -o $@ -w+all

$(BUILD_DIR)/%.o: %.cpp | create_build_dir
	$(CXX) -c -o $@ $< $(CXXFLAGS) $(LINK_FLAGS)

$(OUTPUT_FILE): $(UNSTRIPPED_OUTPUT_FILE)
	strip -s $(UNSTRIPPED_OUTPUT_FILE) -o $(OUTPUT_FILE)

$(UNSTRIPPED_OUTPUT_FILE): $(OBJ_LINK_LIST)
	$(CXX) -T $(LINKER_SCRIPT) -o $(UNSTRIPPED_OUTPUT_FILE) $(OBJ_LINK_LIST) $(CXXFLAGS) $(LINK_FLAGS)

.PHONY: create_build_dir
create_build_dir:
	-mkdir $(BUILD_DIR)
	-mkdir $(BUILD_DIR)/drivers $(BUILD_DIR)/interrupts $(BUILD_DIR)/memory-manager $(BUILD_DIR)/crt

.PHONY: clean
clean:
	-rm -rf $(BUILD_DIR)/*
	-rmdir $(BUILD_DIR)
