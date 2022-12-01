#include "common.hpp"
#include "gdt.hpp"
#include "interrupts/idt.hpp"
#include "interrupts/interrupt_handler.hpp"
#include "interrupts/pic.hpp"
#include "drivers/disk/floppy/floppy.hpp"
#include "drivers/pit/pit.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/ps2/keyboard/keyboard.hpp"
#include "drivers/vga/vga.hpp"
#include "memory-manager/manager.hpp"

namespace Kernel {

    constexpr size_t STACK_SIZE = 16384;
    u32 g_stack[STACK_SIZE / sizeof(u32)] = {0};

    extern "C" void _init();
    extern "C" void _fini();

    extern "C" void kernel_early_main();
    [[noreturn]] void kernel_main();

    void kernel_early_main() {
        _init();

        // Update the stack pointer
        asm("movl %0, %%esp;" : : "r"(g_stack + sizeof(g_stack)) :);

        kernel_main();
        _fini();
    }

    void kernel_main() {
        PIT::initialize();

        VGA::initialize();
        VGA::put_string("Hello World!\n\n");

        VGA::put_string("Initializing PIC... ");
        PIC::initialize();
        VGA::put_string("Done!\n");

        VGA::put_string("Initializing GDT... ");
        GDT::initialize();
        GDT::add_entry(0x0000, 0x0000, 0x0000, 0x0000); // NULL
        GDT::add_entry(0xFFFF, 0x0000, 0x9A00, 0x00CF); // KERNEL_CODE
        GDT::add_entry(0xFFFF, 0x0000, 0x9200, 0x00CF); // KERNEL_DATA
        GDT::load_table();
        VGA::put_string("Done!\n");

        VGA::put_string("Initializing IDT... ");
        IDT::initialize();
        IDT::set_entry(0x20, (void*)&PIT::interval_handler, 0x00000008, IDT::IDTGateType::INTERRUPT, true);
        IDT::set_entry(0x21, (void*)&PS2::Keyboard::keyboard_handler, 0x00000008, IDT::IDTGateType::INTERRUPT, true);
        IDT::set_entry(0x26, (void*)&FloppyDisk::floppy_handler, 0x00000008, IDT::IDTGateType::INTERRUPT, true);
        IDT::load_table();
        VGA::put_string("Done!\n\n");

        VGA::put_string("Initializing PS/2 Controller... ");
        const auto errorOr = PS2::initialize();
        if (errorOr.is_error()) {
            VGA::put_string(get_error_string(errorOr.get_error()));
            VGA::put_string(" Failed :(\n");
            KERNEL_STOP();
        }
        VGA::put_string("Done!\n");

        VGA::put_string("Initializing PS/2 Keyboard... ");
        if (PS2::Keyboard::initialize().is_error()) {
            VGA::put_string("Failed :(\n");
            KERNEL_STOP();
        }
        VGA::put_string("Done! \n\n");

        VGA::put_string("Enabling interrupts... ");
        enable_interrupts();
        VGA::put_string("Done!\n");

        VGA::put_string("CMOS: ");
        VGA::put_hex(read_cmos(0x10));
        VGA::new_line();

        VGA::put_string("Intializing Floppy Disk... ");
        if (FloppyDisk::initialize().is_error()) {
            VGA::put_string("Failed :(\n");
            KERNEL_STOP();
        }
        VGA::put_string("Done!\n");

        //*
        u8 buffer[FloppyDisk::SECTOR_SIZE];
        if (FloppyDisk::read_data(0, 80, 1, buffer).is_error()) {
            VGA::put_string("Read failed :(\n");
            KERNEL_STOP();
        }

        for (size_t i = 0; i < FloppyDisk::SECTOR_SIZE; i++) {
            VGA::put_char(buffer[i]);
        }
        VGA::new_line();
        //*/

        VGA::put_string("Intializing Memory Manager... ");
        if (MemoryManager::initialize().is_error()) {
            VGA::put_string("Failed :(\n");
            KERNEL_STOP();
        }
        VGA::put_string("Done!\n");

        MemoryManager::print_heap_information();

        char** stringArray = new char*[4];
        for (size_t i = 0; i < 4; i++) {
            stringArray[i] = new char[32 * (i + 1)];
            stringArray[i][0] = 'A' + char(i);
            stringArray[i][1] = '\0';
        }

        delete[] stringArray[0];
        delete[] stringArray[3];
        delete[] stringArray[1];
        delete[] stringArray[2];
        delete[] stringArray;

        while (true) {
            for (auto e = PS2::Keyboard::poll_event(); !e.is_error(); e = PS2::Keyboard::poll_event()) {
                const auto event = e.get_value();

                if (event.event == PS2::Keyboard::KeyEvent::PRESSED) {
                    switch(event.key) {
                        case PS2::Keyboard::Keycode::KEYCODE_BACKSPACE:
                            VGA::put_string("\b \b");
                            break;
                        default: {
                            const char c = PS2::Keyboard::get_keycode_char(event.key);
                            if (VGA::get_cursor_pos().x < 79 && c != '\0') {
                                VGA::put_char(c);
                            }
                            break;
                        }
                    }
                }
            }
            KERNEL_HALT();
        }

        KERNEL_STOP();
    }

}
