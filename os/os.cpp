#include "common.hpp"
#include "gdt.hpp"
#include "interrupts/idt.hpp"
#include "interrupts/interrupt_handler.hpp"
#include "interrupts/pic.hpp"
#include "drivers/pit.hpp"
#include "drivers/ps2.hpp"
#include "drivers/ps2_keyboard.hpp"
#include "drivers/vga.hpp"

namespace Kernel {

    u64* const MEMORY_INFORMATION_TABLE = (u64*) 0x7000;

    struct MemoryRange {
        u32 baseAddress;
        u32 regionLength;
    };

    struct MemoryRangeTable {
        MemoryRange entries[32];
        u32 entryCount;
    };

    static MemoryRangeTable MEMORY_USABLE_RANGE_TABLE;

    void initialize_memory_range() {
        MEMORY_USABLE_RANGE_TABLE.entryCount = 0;
        for (u64* p = MEMORY_INFORMATION_TABLE; ; p += 3) {
            // If we have reached the NULL entry or there is no more space in the table then stop
            if ((p[0] == 0 && p[1] == 0 && p[2] == 0) || MEMORY_USABLE_RANGE_TABLE.entryCount >= 32) {
                break;
            }

            const MemoryRange range = { static_cast<u32>(p[0] & 0xFFFFFFFF) , static_cast<u32>(p[1] & 0xFFFFFFFF) };
            const u32 regionType = p[2] & 0xFFFFFFFF;
            if (regionType == 1) {
                MEMORY_USABLE_RANGE_TABLE.entries[MEMORY_USABLE_RANGE_TABLE.entryCount] = range;
                MEMORY_USABLE_RANGE_TABLE.entryCount++;

                VGA::put_hex(range.baseAddress);
                VGA::put_char(' ');

                VGA::put_hex(range.regionLength);
                VGA::new_line();
            }
        }
    }

    extern "C" void kernel_main() {
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
        IDT::load_table();
        VGA::put_string("Done!\n\n");

        VGA::put_string("Initializing PS/2 Controller... ");
        const auto errorOr = PS2::initialize();
        if (errorOr.is_error()) {
            VGA::put_hex(errorOr.get_error());
            VGA::put_string(" Failed :(\n");
            disable_interrupts();
            KERNEL_HALT();
        }
        VGA::put_string("Done!\n");

        VGA::put_string("Initializing PS/2 Keyboard... ");
        if (PS2::Keyboard::initialize().is_error()) {
            VGA::put_string("Failed :(\n");
            disable_interrupts();
            KERNEL_HALT();
        }
        VGA::put_string("Done! \n\n");

        VGA::put_string("Enabling interrupts\n\n");
        enable_interrupts();

        VGA::put_string("Memory Ranges\n---------------------\n");
        initialize_memory_range();
        VGA::new_line();

        VGA::put_string("Done!\n");
        VGA::put_hex(read_cmos(0x10));
        VGA::new_line();

        /*
        while (true) {
            for (auto e = PS2::Keyboard::poll_event(); !e.is_error(); e = PS2::Keyboard::poll_event()) {
                const auto event = e.get_value();
                VGA::put_string(PS2::Keyboard::get_keycode_string(event.key));
                if (event.event == PS2::Keyboard::KeyEvent::PRESSED) {
                    VGA::put_string(" pressed\n");
                }
                else {
                    VGA::put_string(" released\n");
                }
            }
            KERNEL_HALT();
        }
        //*/

        //*
        while (true) {
            VGA::put_string("Sleeping Zzz...\n");
            sleep(3000);
            VGA::put_string("Woke up\n");
        }
        //*/
    }

}
