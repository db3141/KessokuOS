#ifndef PS2_KEYBOARD_INCLUDED
#define PS2_KEYBOARD_INCLUDED

#include "error_code_groups.hpp"
#include "sznnlib/error_or.hpp"

namespace Kernel::PS2::Keyboard {

    enum Error : int {
        ERROR_KEYBOARD_SELF_TEST_FAILED = ErrorCodeGroup::DRIVERS_PS2_KEYBOARD,
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

    SZNN::ErrorOr<void> initialize();

    SZNN::ErrorOr<KeyboardEvent> poll_event();

    bool is_key_pressed(Keycode t_key);

    INTERRUPT_HANDLER void keyboard_handler(InterruptHandler::InterruptFrame* t_frame);

}

#endif
