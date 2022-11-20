#ifndef PS2_KEYBOARD_INCLUDED
#define PS2_KEYBOARD_INCLUDED

#include "error_code_groups.hpp"
#include "data/error_or.hpp"

namespace Kernel::PS2::Keyboard {

    enum Error : int {
        ERROR_KEYBOARD_SELF_TEST_FAILED = ErrorCodeGroup::get_id(ErrorCodeGroup::Group::DRIVERS_PS2_KEYBOARD),
        ERROR_KEYBOARD_RESEND_LIMIT_REACHED,
        ERROR_KEYBOARD_DEVICE_IS_NOT_A_KEYBOARD
    };

    enum Keycode : u8 {
        KEYCODE_ESCAPE = 0b000'00000,
        KEYCODE_F1,
        KEYCODE_F2,
        KEYCODE_F3,
        KEYCODE_F4,
        KEYCODE_F5,
        KEYCODE_F6,
        KEYCODE_F7,
        KEYCODE_F8,
        KEYCODE_F9,
        KEYCODE_F10,
        KEYCODE_F11,
        KEYCODE_F12,
        KEYCODE_PRINT_SCREEN,
        KEYCODE_SCROLL_LOCK,
        KEYCODE_PAUSE_BREAK,

        KEYCODE_BACKTICK = 0b001'00000,
        KEYCODE_1,
        KEYCODE_2,
        KEYCODE_3,
        KEYCODE_4,
        KEYCODE_5,
        KEYCODE_6,
        KEYCODE_7,
        KEYCODE_8,
        KEYCODE_9,
        KEYCODE_0,
        KEYCODE_MINUS,
        KEYCODE_EQUALS,
        KEYCODE_BACKSPACE,
        KEYCODE_INSERT,
        KEYCODE_HOME,
        KEYCODE_PAGE_UP,
        KEYCODE_NUMBER_LOCK,
        KEYCODE_KEYPAD_SLASH,
        KEYCODE_KEYPAD_MULTIPLY,
        KEYCODE_KEYPAD_MINUS,

        KEYCODE_TAB = 0b010'00000,
        KEYCODE_Q,
        KEYCODE_W,
        KEYCODE_E,
        KEYCODE_R,
        KEYCODE_T,
        KEYCODE_Y,
        KEYCODE_U,
        KEYCODE_I,
        KEYCODE_O,
        KEYCODE_P,
        KEYCODE_LEFT_BRACKET,
        KEYCODE_RIGHT_BRACKET,
        KEYCODE_ENTER,
        KEYCODE_DELETE,
        KEYCODE_END,
        KEYCODE_PAGE_DOWN,
        KEYCODE_KEYPAD_7,
        KEYCODE_KEYPAD_8,
        KEYCODE_KEYPAD_9,
        KEYCODE_KEYPAD_PLUS,

        KEYCODE_CAPSLOCK = 0b011'00000,
        KEYCODE_A,
        KEYCODE_S,
        KEYCODE_D,
        KEYCODE_F,
        KEYCODE_G,
        KEYCODE_H,
        KEYCODE_J,
        KEYCODE_K,
        KEYCODE_L,
        KEYCODE_SEMICOLON,
        KEYCODE_APOSTROPHE,
        KEYCODE_HASH,
        KEYCODE_KEYPAD_4,
        KEYCODE_KEYPAD_5,
        KEYCODE_KEYPAD_6,

        KEYCODE_LEFT_SHIFT = 0b100'00000,
        KEYCODE_BACKSLASH,
        KEYCODE_Z,
        KEYCODE_X,
        KEYCODE_C,
        KEYCODE_V,
        KEYCODE_B,
        KEYCODE_N,
        KEYCODE_M,
        KEYCODE_COMMA,
        KEYCODE_FULLSTOP,
        KEYCODE_SLASH,
        KEYCODE_RIGHT_SHIFT,
        KEYCODE_UP_ARROW,
        KEYCODE_KEYPAD_1,
        KEYCODE_KEYPAD_2,
        KEYCODE_KEYPAD_3,
        KEYCODE_KEYPAD_ENTER,

        KEYCODE_LEFT_CTRL = 0b101'00000,
        KEYCODE_LEFT_SUPER,
        KEYCODE_LEFT_ALT,
        KEYCODE_SPACE,
        KEYCODE_RIGHT_ALT,
        KEYCODE_RIGHT_SUPER,
        KEYCODE_MENU,
        KEYCODE_RIGHT_CTRL,
        KEYCODE_LEFT_ARROW,
        KEYCODE_DOWN_ARROW,
        KEYCODE_RIGHT_ARROW,
        KEYCODE_KEYPAD_0,
        KEYCODE_KEYPAD_FULLSTOP,

        KEYCODE_UNKNOWN = 0b111'11111
    };

    enum class KeyEvent {
        PRESSED,
        RELEASED,
    };

    struct KeyboardEvent {
        Keycode key;
        KeyEvent event;
    };

    constexpr const char* get_keycode_string(Keycode t_keycode) {
        switch (t_keycode) {
            case KEYCODE_A:
                return "a";
            case KEYCODE_B:
                return "b";
            case KEYCODE_C:
                return "c";
            case KEYCODE_D:
                return "d";
            case KEYCODE_E:
                return "e";
            case KEYCODE_F:
                return "f";
            case KEYCODE_G:
                return "g";
            case KEYCODE_H:
                return "h";
            case KEYCODE_I:
                return "i";
            case KEYCODE_J:
                return "j";
            case KEYCODE_K:
                return "k";
            case KEYCODE_L:
                return "l";
            case KEYCODE_M:
                return "m";
            case KEYCODE_N:
                return "n";
            case KEYCODE_O:
                return "o";
            case KEYCODE_P:
                return "p";
            case KEYCODE_Q:
                return "q";
            case KEYCODE_R:
                return "r";
            case KEYCODE_S:
                return "s";
            case KEYCODE_T:
                return "t";
            case KEYCODE_U:
                return "u";
            case KEYCODE_V:
                return "v";
            case KEYCODE_W:
                return "w";
            case KEYCODE_X:
                return "x";
            case KEYCODE_Y:
                return "y";
            case KEYCODE_Z:
                return "z";

            case KEYCODE_ESCAPE:
                return "ESC";
            case KEYCODE_F1:
                return "F1";
            case KEYCODE_F2:
                return "F2";
            case KEYCODE_F3:
                return "F3";
            case KEYCODE_F4:
                return "F4";
            case KEYCODE_F5:
                return "F5";
            case KEYCODE_F6:
                return "F6";
            case KEYCODE_F7:
                return "F7";
            case KEYCODE_F8:
                return "F8";
            case KEYCODE_F9:
                return "F9";
            case KEYCODE_F10:
                return "F10";
            case KEYCODE_F11:
                return "F11";
            case KEYCODE_F12:
                return "F12";
            case KEYCODE_PRINT_SCREEN:
                return "PRTSCR";
            case KEYCODE_SCROLL_LOCK:
                return "SCRLCK";
            case KEYCODE_PAUSE_BREAK:
                return "PSEBRK";

            case KEYCODE_BACKTICK:
                return "`";
            case KEYCODE_0:
                return "0";
            case KEYCODE_1:
                return "1";
            case KEYCODE_2:
                return "2";
            case KEYCODE_3:
                return "3";
            case KEYCODE_4:
                return "4";
            case KEYCODE_5:
                return "5";
            case KEYCODE_6:
                return "6";
            case KEYCODE_7:
                return "7";
            case KEYCODE_8:
                return "8";
            case KEYCODE_9:
                return "9";
            case KEYCODE_MINUS:
                return "-";
            case KEYCODE_EQUALS:
                return "=";
            case KEYCODE_BACKSPACE:
                return "BCKSPC";
            case KEYCODE_INSERT:
                return "INSERT";
            case KEYCODE_HOME:
                return "HOME";
            case KEYCODE_PAGE_UP:
                return "PGUP";
            case KEYCODE_NUMBER_LOCK:
                return "NUMLCK";
            case KEYCODE_KEYPAD_SLASH:
                return "KPDIV";
            case KEYCODE_KEYPAD_MULTIPLY:
                return "KPMUL";
            case KEYCODE_KEYPAD_MINUS:
                return "KPMIN";

            case KEYCODE_TAB:
                return "TAB";
            case KEYCODE_LEFT_BRACKET:
                return "[";
            case KEYCODE_RIGHT_BRACKET:
                return "]";
            case KEYCODE_ENTER:
                return "ENTER";
            case KEYCODE_DELETE:
                return "DELETE";
            case KEYCODE_END:
                return "END";
            case KEYCODE_PAGE_DOWN:
                return "PGDN";
            case KEYCODE_KEYPAD_PLUS:
                return "KPPLUS";

            case KEYCODE_CAPSLOCK:
                return "CPSLCK";
            case KEYCODE_SEMICOLON:
                return ";";
            case KEYCODE_APOSTROPHE:
                return "'";
            case KEYCODE_HASH:
                return "#";

            case KEYCODE_LEFT_SHIFT:
                return "LSHIFT";
            case KEYCODE_BACKSLASH:
                return "\\";
            case KEYCODE_COMMA:
                return ",";
            case KEYCODE_FULLSTOP:
                return ".";
            case KEYCODE_SLASH:
                return "/";
            case KEYCODE_RIGHT_SHIFT:
                return "RSHIFT";
            case KEYCODE_UP_ARROW:
                return "UARROW";
            case KEYCODE_KEYPAD_ENTER:
                return "KPENTR";

            case KEYCODE_LEFT_CTRL:
                return "LCTRL";
            case KEYCODE_LEFT_SUPER:
                return "LSUPER";
            case KEYCODE_LEFT_ALT:
                return "LALT";
            case KEYCODE_SPACE:
                return "SPACE";
            case KEYCODE_RIGHT_ALT:
                return "RALT";
            case KEYCODE_RIGHT_SUPER:
                return "RSUPER";
            case KEYCODE_MENU:
                return "MENU";
            case KEYCODE_RIGHT_CTRL:
                return "RCTRL";
            case KEYCODE_LEFT_ARROW:
                return "LARROW";
            case KEYCODE_DOWN_ARROW:
                return "DARROW";
            case KEYCODE_RIGHT_ARROW:
                return "RARROW";
            case KEYCODE_KEYPAD_FULLSTOP:
                return "KPDOT";

            case KEYCODE_KEYPAD_0:
                return "KP0";
            case KEYCODE_KEYPAD_1:
                return "KP1";
            case KEYCODE_KEYPAD_2:
                return "KP2";
            case KEYCODE_KEYPAD_3:
                return "KP3";
            case KEYCODE_KEYPAD_4:
                return "KP4";
            case KEYCODE_KEYPAD_5:
                return "KP5";
            case KEYCODE_KEYPAD_6:
                return "KP6";
            case KEYCODE_KEYPAD_7:
                return "KP7";
            case KEYCODE_KEYPAD_8:
                return "KP8";
            case KEYCODE_KEYPAD_9:
                return "KP9";

            default:
                return "BADKEY";
        }
    }

    constexpr char get_keycode_char(Keycode t_keycode) {
        switch (t_keycode) {
            case KEYCODE_A:
                return 'a';
            case KEYCODE_B:
                return 'b';
            case KEYCODE_C:
                return 'c';
            case KEYCODE_D:
                return 'd';
            case KEYCODE_E:
                return 'e';
            case KEYCODE_F:
                return 'f';
            case KEYCODE_G:
                return 'g';
            case KEYCODE_H:
                return 'h';
            case KEYCODE_I:
                return 'i';
            case KEYCODE_J:
                return 'j';
            case KEYCODE_K:
                return 'k';
            case KEYCODE_L:
                return 'l';
            case KEYCODE_M:
                return 'm';
            case KEYCODE_N:
                return 'n';
            case KEYCODE_O:
                return 'o';
            case KEYCODE_P:
                return 'p';
            case KEYCODE_Q:
                return 'q';
            case KEYCODE_R:
                return 'r';
            case KEYCODE_S:
                return 's';
            case KEYCODE_T:
                return 't';
            case KEYCODE_U:
                return 'u';
            case KEYCODE_V:
                return 'v';
            case KEYCODE_W:
                return 'w';
            case KEYCODE_X:
                return 'x';
            case KEYCODE_Y:
                return 'y';
            case KEYCODE_Z:
                return 'z';

            case KEYCODE_BACKTICK:
                return '`';
            case KEYCODE_0:
                return '0';
            case KEYCODE_1:
                return '1';
            case KEYCODE_2:
                return '2';
            case KEYCODE_3:
                return '3';
            case KEYCODE_4:
                return '4';
            case KEYCODE_5:
                return '5';
            case KEYCODE_6:
                return '6';
            case KEYCODE_7:
                return '7';
            case KEYCODE_8:
                return '8';
            case KEYCODE_9:
                return '9';
            case KEYCODE_MINUS:
                return '-';
            case KEYCODE_EQUALS:
                return '=';
            case KEYCODE_KEYPAD_SLASH:
                return '/';
            case KEYCODE_KEYPAD_MULTIPLY:
                return '*';
            case KEYCODE_KEYPAD_MINUS:
                return '-';

            case KEYCODE_TAB:
                return '\t';
            case KEYCODE_LEFT_BRACKET:
                return '[';
            case KEYCODE_RIGHT_BRACKET:
                return ']';
            case KEYCODE_KEYPAD_PLUS:
                return '+';

            case KEYCODE_SEMICOLON:
                return ';';
            case KEYCODE_APOSTROPHE:
                return '\'';
            case KEYCODE_HASH:
                return '#';

            case KEYCODE_BACKSLASH:
                return '\\';
            case KEYCODE_COMMA:
                return ',';
            case KEYCODE_FULLSTOP:
                return '.';
            case KEYCODE_SLASH:
                return '/';

            case KEYCODE_SPACE:
                return ' ';
            case KEYCODE_KEYPAD_FULLSTOP:
                return '.';

            case KEYCODE_KEYPAD_0:
                return '0';
            case KEYCODE_KEYPAD_1:
                return '1';
            case KEYCODE_KEYPAD_2:
                return '2';
            case KEYCODE_KEYPAD_3:
                return '3';
            case KEYCODE_KEYPAD_4:
                return '4';
            case KEYCODE_KEYPAD_5:
                return '5';
            case KEYCODE_KEYPAD_6:
                return '6';
            case KEYCODE_KEYPAD_7:
                return '7';
            case KEYCODE_KEYPAD_8:
                return '8';
            case KEYCODE_KEYPAD_9:
                return '9';

            default:
                return '\0';
        }
    }

    Data::ErrorOr<void> initialize();

    Data::ErrorOr<KeyboardEvent> poll_event();

    bool is_key_pressed(Keycode t_key);

    INTERRUPT_HANDLER void keyboard_handler(InterruptHandler::InterruptFrame* t_frame);

}

#endif
