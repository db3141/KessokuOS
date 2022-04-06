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

    constexpr const char* get_keycode_string(PS2::Keyboard::Keycode t_keycode) {
        switch (t_keycode) {
            case PS2::Keyboard::KEYCODE_A:
                return "a";
            case PS2::Keyboard::KEYCODE_B:
                return "b";
            case PS2::Keyboard::KEYCODE_C:
                return "c";
            case PS2::Keyboard::KEYCODE_D:
                return "d";
            case PS2::Keyboard::KEYCODE_E:
                return "e";
            case PS2::Keyboard::KEYCODE_F:
                return "f";
            case PS2::Keyboard::KEYCODE_G:
                return "g";
            case PS2::Keyboard::KEYCODE_H:
                return "h";
            case PS2::Keyboard::KEYCODE_I:
                return "i";
            case PS2::Keyboard::KEYCODE_J:
                return "j";
            case PS2::Keyboard::KEYCODE_K:
                return "k";
            case PS2::Keyboard::KEYCODE_L:
                return "l";
            case PS2::Keyboard::KEYCODE_M:
                return "m";
            case PS2::Keyboard::KEYCODE_N:
                return "n";
            case PS2::Keyboard::KEYCODE_O:
                return "o";
            case PS2::Keyboard::KEYCODE_P:
                return "p";
            case PS2::Keyboard::KEYCODE_Q:
                return "q";
            case PS2::Keyboard::KEYCODE_R:
                return "r";
            case PS2::Keyboard::KEYCODE_S:
                return "s";
            case PS2::Keyboard::KEYCODE_T:
                return "t";
            case PS2::Keyboard::KEYCODE_U:
                return "u";
            case PS2::Keyboard::KEYCODE_V:
                return "v";
            case PS2::Keyboard::KEYCODE_W:
                return "w";
            case PS2::Keyboard::KEYCODE_X:
                return "x";
            case PS2::Keyboard::KEYCODE_Y:
                return "y";
            case PS2::Keyboard::KEYCODE_Z:
                return "z";

            case PS2::Keyboard::KEYCODE_ESCAPE:
                return "ESC";
            case PS2::Keyboard::KEYCODE_F1:
                return "F1";
            case PS2::Keyboard::KEYCODE_F2:
                return "F2";
            case PS2::Keyboard::KEYCODE_F3:
                return "F3";
            case PS2::Keyboard::KEYCODE_F4:
                return "F4";
            case PS2::Keyboard::KEYCODE_F5:
                return "F5";
            case PS2::Keyboard::KEYCODE_F6:
                return "F6";
            case PS2::Keyboard::KEYCODE_F7:
                return "F7";
            case PS2::Keyboard::KEYCODE_F8:
                return "F8";
            case PS2::Keyboard::KEYCODE_F9:
                return "F9";
            case PS2::Keyboard::KEYCODE_F10:
                return "F10";
            case PS2::Keyboard::KEYCODE_F11:
                return "F11";
            case PS2::Keyboard::KEYCODE_F12:
                return "F12";
            case PS2::Keyboard::KEYCODE_PRINT_SCREEN:
                return "PRTSCR";
            case PS2::Keyboard::KEYCODE_SCROLL_LOCK:
                return "SCRLCK";
            case PS2::Keyboard::KEYCODE_PAUSE_BREAK:
                return "PSEBRK";

            case PS2::Keyboard::KEYCODE_BACKTICK:
                return "`";
            case PS2::Keyboard::KEYCODE_0:
                return "0";
            case PS2::Keyboard::KEYCODE_1:
                return "1";
            case PS2::Keyboard::KEYCODE_2:
                return "2";
            case PS2::Keyboard::KEYCODE_3:
                return "3";
            case PS2::Keyboard::KEYCODE_4:
                return "4";
            case PS2::Keyboard::KEYCODE_5:
                return "5";
            case PS2::Keyboard::KEYCODE_6:
                return "6";
            case PS2::Keyboard::KEYCODE_7:
                return "7";
            case PS2::Keyboard::KEYCODE_8:
                return "8";
            case PS2::Keyboard::KEYCODE_9:
                return "9";
            case PS2::Keyboard::KEYCODE_MINUS:
                return "-";
            case PS2::Keyboard::KEYCODE_EQUALS:
                return "=";
            case PS2::Keyboard::KEYCODE_BACKSPACE:
                return "BCKSPC";
            case PS2::Keyboard::KEYCODE_INSERT:
                return "INSERT";
            case PS2::Keyboard::KEYCODE_HOME:
                return "HOME";
            case PS2::Keyboard::KEYCODE_PAGE_UP:
                return "PGUP";
            case PS2::Keyboard::KEYCODE_NUMBER_LOCK:
                return "NUMLCK";
            case PS2::Keyboard::KEYCODE_KEYPAD_SLASH:
                return "KPDIV";
            case PS2::Keyboard::KEYCODE_KEYPAD_MULTIPLY:
                return "KPMUL";
            case PS2::Keyboard::KEYCODE_KEYPAD_MINUS:
                return "KPMIN";

            case PS2::Keyboard::KEYCODE_TAB:
                return "TAB";
            case PS2::Keyboard::KEYCODE_LEFT_BRACKET:
                return "[";
            case PS2::Keyboard::KEYCODE_RIGHT_BRACKET:
                return "]";
            case PS2::Keyboard::KEYCODE_ENTER:
                return "ENTER";
            case PS2::Keyboard::KEYCODE_DELETE:
                return "DELETE";
            case PS2::Keyboard::KEYCODE_END:
                return "END";
            case PS2::Keyboard::KEYCODE_PAGE_DOWN:
                return "PGDN";
            case PS2::Keyboard::KEYCODE_KEYPAD_PLUS:
                return "KPPLUS";

            case PS2::Keyboard::KEYCODE_CAPSLOCK:
                return "CPSLCK";
            case PS2::Keyboard::KEYCODE_SEMICOLON:
                return ";";
            case PS2::Keyboard::KEYCODE_APOSTROPHE:
                return "'";
            case PS2::Keyboard::KEYCODE_HASH:
                return "#";

            case PS2::Keyboard::KEYCODE_LEFT_SHIFT:
                return "LSHIFT";
            case PS2::Keyboard::KEYCODE_BACKSLASH:
                return "\\";
            case PS2::Keyboard::KEYCODE_COMMA:
                return ",";
            case PS2::Keyboard::KEYCODE_FULLSTOP:
                return ".";
            case PS2::Keyboard::KEYCODE_SLASH:
                return "/";
            case PS2::Keyboard::KEYCODE_RIGHT_SHIFT:
                return "RSHIFT";
            case PS2::Keyboard::KEYCODE_UP_ARROW:
                return "UARROW";
            case PS2::Keyboard::KEYCODE_KEYPAD_ENTER:
                return "KPENTR";

            case PS2::Keyboard::KEYCODE_LEFT_CTRL:
                return "LCTRL";
            case PS2::Keyboard::KEYCODE_LEFT_SUPER:
                return "LSUPER";
            case PS2::Keyboard::KEYCODE_LEFT_ALT:
                return "LALT";
            case PS2::Keyboard::KEYCODE_SPACE:
                return "SPACE";
            case PS2::Keyboard::KEYCODE_RIGHT_ALT:
                return "RALT";
            case PS2::Keyboard::KEYCODE_RIGHT_SUPER:
                return "RSUPER";
            case PS2::Keyboard::KEYCODE_MENU:
                return "MENU";
            case PS2::Keyboard::KEYCODE_RIGHT_CTRL:
                return "RCTRL";
            case PS2::Keyboard::KEYCODE_LEFT_ARROW:
                return "LARROW";
            case PS2::Keyboard::KEYCODE_DOWN_ARROW:
                return "DARROW";
            case PS2::Keyboard::KEYCODE_RIGHT_ARROW:
                return "RARROW";
            case PS2::Keyboard::KEYCODE_KEYPAD_FULLSTOP:
                return "KPDOT";

            case PS2::Keyboard::KEYCODE_KEYPAD_0:
                return "KP0";
            case PS2::Keyboard::KEYCODE_KEYPAD_1:
                return "KP1";
            case PS2::Keyboard::KEYCODE_KEYPAD_2:
                return "KP2";
            case PS2::Keyboard::KEYCODE_KEYPAD_3:
                return "KP3";
            case PS2::Keyboard::KEYCODE_KEYPAD_4:
                return "KP4";
            case PS2::Keyboard::KEYCODE_KEYPAD_5:
                return "KP5";
            case PS2::Keyboard::KEYCODE_KEYPAD_6:
                return "KP6";
            case PS2::Keyboard::KEYCODE_KEYPAD_7:
                return "KP7";
            case PS2::Keyboard::KEYCODE_KEYPAD_8:
                return "KP8";
            case PS2::Keyboard::KEYCODE_KEYPAD_9:
                return "KP9";

            default:
                return "BADKEY";
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
        for (size_t i = 0; i < 256; i++) {
            if (i == 0x20) {
                IDT::add_entry((void*)&PIT::interval_handler, 0x00000008, IDT::IDTGateType::INTERRUPT, true);
            }
            if (i == 0x21) {
                IDT::add_entry((void*)&PS2::Keyboard::keyboard_handler, 0x00000008, IDT::IDTGateType::INTERRUPT, true);
            }
            else {
                IDT::add_entry((void*)&InterruptHandler::interrupt_handler, 0x00000008, IDT::IDTGateType::INTERRUPT, true);
            }
        }
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
                VGA::put_string(get_keycode_string(event.key));
                if (event.event == PS2::Keyboard::KeyEvent::PRESSED) {
                    VGA::put_string(" pressed\n");
                }
                else {
                    VGA::put_string(" released\n");
                }
            }
            KERNEL_HALT();
        }
        */

        while (true) {
            VGA::put_string("Sleeping Zzz...\n");
            sleep(3000);
            VGA::put_string("Woke up\n");
        }
    }

}
