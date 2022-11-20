#include "common.hpp"
#include "data/queue.hpp"
#include "interrupts/pic.hpp"
#include "ps2.hpp"
#include "ps2_keyboard.hpp"
#include "vga.hpp"

namespace Kernel::PS2::Keyboard {
    
    constexpr u32 SEND_COMMAND_RESPONSE_RESEND_LIMIT = 3;
    constexpr size_t KEYBOARD_EVENT_QUEUE_SIZE = 32;

    static bool KEYBOARD_KEY_STATE[256] = {0};
    static Data::Queue<KeyboardEvent, KEYBOARD_EVENT_QUEUE_SIZE> KEYBOARD_EVENT_QUEUE;

    enum Command : u8 {
        COMMAND_ENABLE_SCANNING = 0xF4,

        COMMAND_RESET_AND_SELF_TEST = 0xFF
    };

    enum Responses : u8 {
        RESPONSE_ACK = 0xFA,
        RESPONSE_RESEND = 0xFE,
    };

    // Send command until an response isn't a resend or the attempt limit is reached
    Data::ErrorOr<u8> resend_until_success_or_timeout(u8 t_command);

    Keycode parse_scancode(KeyEvent& r_event);
    
    constexpr bool is_keyboard(DeviceType t_type) {
        return (
            (t_type == DeviceType::AT_KEYBOARD) ||
            (t_type == DeviceType::MF2_KEYBOARD_TRANSLATION_ENABLED) ||
            (t_type == DeviceType::MF2_KEYBOARD)
        );
    }

    Data::ErrorOr<void> initialize() {
        const DeviceType type = TRY(get_first_port_device_type());
        if (!is_keyboard(type)) {
            VGA::put_char('\'');
            VGA::put_string(device_type_string(type));
            VGA::put_string("' is not a keyboard\n");
            return ERROR_KEYBOARD_DEVICE_IS_NOT_A_KEYBOARD;
        }
        
        TRY(resend_until_success_or_timeout(COMMAND_RESET_AND_SELF_TEST));

        const auto selfTestResult = TRY(get_response());
        if (selfTestResult != 0xAA) {
            return ERROR_KEYBOARD_SELF_TEST_FAILED;
        }
        TRY(resend_until_success_or_timeout(COMMAND_ENABLE_SCANNING));

        return Data::ErrorOr<void>();
    }

    Data::ErrorOr<KeyboardEvent> poll_event() {
        return KEYBOARD_EVENT_QUEUE.pop_front();
    }

    Data::ErrorOr<u8> resend_until_success_or_timeout(u8 t_command) {
        for (u32 i = 0; i < 3; i++) {
            TRY(send_to_device(t_command));

            const u8 response = TRY(get_response());
            if (response != RESPONSE_RESEND) {
                return response;
            }
        }

        return ERROR_KEYBOARD_RESEND_LIMIT_REACHED;
    }

    Keycode parse_scancode(KeyEvent& r_event) {
        constexpr Keycode SCANCODE_MAP_SINGLE_BYTE[256] = {
            KEYCODE_UNKNOWN, /* 0x00 */
            KEYCODE_F9, /* 0x01 */
            KEYCODE_UNKNOWN, /* 0x02 */
            KEYCODE_F5, /* 0x03 */
            KEYCODE_F3, /* 0x04 */
            KEYCODE_F1, /* 0x05 */
            KEYCODE_F2, /* 0x06 */
            KEYCODE_F12, /* 0x07 */
            KEYCODE_UNKNOWN, /* 0x08 */
            KEYCODE_F10, /* 0x09 */
            KEYCODE_F8, /* 0x0a */
            KEYCODE_F6, /* 0x0b */
            KEYCODE_F4, /* 0x0c */
            KEYCODE_TAB, /* 0x0d */
            KEYCODE_BACKTICK, /* 0x0e */
            KEYCODE_UNKNOWN, /* 0x0f */
            KEYCODE_UNKNOWN, /* 0x10 */
            KEYCODE_LEFT_ALT, /* 0x11 */
            KEYCODE_LEFT_SHIFT, /* 0x12 */
            KEYCODE_UNKNOWN, /* 0x13 */
            KEYCODE_LEFT_CTRL, /* 0x14 */
            KEYCODE_Q, /* 0x15 */
            KEYCODE_1, /* 0x16 */
            KEYCODE_UNKNOWN, /* 0x17 */
            KEYCODE_UNKNOWN, /* 0x18 */
            KEYCODE_UNKNOWN, /* 0x19 */
            KEYCODE_Z, /* 0x1a */
            KEYCODE_S, /* 0x1b */
            KEYCODE_A, /* 0x1c */
            KEYCODE_W, /* 0x1d */
            KEYCODE_2, /* 0x1e */
            KEYCODE_UNKNOWN, /* 0x1f */
            KEYCODE_UNKNOWN, /* 0x20 */
            KEYCODE_C, /* 0x21 */
            KEYCODE_X, /* 0x22 */
            KEYCODE_D, /* 0x23 */
            KEYCODE_E, /* 0x24 */
            KEYCODE_4, /* 0x25 */
            KEYCODE_3, /* 0x26 */
            KEYCODE_UNKNOWN, /* 0x27 */
            KEYCODE_UNKNOWN, /* 0x28 */
            KEYCODE_SPACE, /* 0x29 */
            KEYCODE_V, /* 0x2a */
            KEYCODE_F, /* 0x2b */
            KEYCODE_T, /* 0x2c */
            KEYCODE_R, /* 0x2d */
            KEYCODE_5, /* 0x2e */
            KEYCODE_UNKNOWN, /* 0x2f */
            KEYCODE_UNKNOWN, /* 0x30 */
            KEYCODE_N, /* 0x31 */
            KEYCODE_B, /* 0x32 */
            KEYCODE_H, /* 0x33 */
            KEYCODE_G, /* 0x34 */
            KEYCODE_Y, /* 0x35 */
            KEYCODE_6, /* 0x36 */
            KEYCODE_UNKNOWN, /* 0x37 */
            KEYCODE_UNKNOWN, /* 0x38 */
            KEYCODE_UNKNOWN, /* 0x39 */
            KEYCODE_M, /* 0x3a */
            KEYCODE_J, /* 0x3b */
            KEYCODE_U, /* 0x3c */
            KEYCODE_7, /* 0x3d */
            KEYCODE_8, /* 0x3e */
            KEYCODE_UNKNOWN, /* 0x3f */
            KEYCODE_UNKNOWN, /* 0x40 */
            KEYCODE_COMMA, /* 0x41 */
            KEYCODE_K, /* 0x42 */
            KEYCODE_I, /* 0x43 */
            KEYCODE_O, /* 0x44 */
            KEYCODE_0, /* 0x45 */
            KEYCODE_9, /* 0x46 */
            KEYCODE_UNKNOWN, /* 0x47 */
            KEYCODE_UNKNOWN, /* 0x48 */
            KEYCODE_FULLSTOP, /* 0x49 */
            KEYCODE_SLASH, /* 0x4a */
            KEYCODE_L, /* 0x4b */
            KEYCODE_SEMICOLON, /* 0x4c */
            KEYCODE_P, /* 0x4d */
            KEYCODE_MINUS, /* 0x4e */
            KEYCODE_UNKNOWN, /* 0x4f */
            KEYCODE_UNKNOWN, /* 0x50 */
            KEYCODE_UNKNOWN, /* 0x51 */
            KEYCODE_APOSTROPHE, /* 0x52 */
            KEYCODE_UNKNOWN, /* 0x53 */
            KEYCODE_LEFT_BRACKET, /* 0x54 */
            KEYCODE_EQUALS, /* 0x55 */
            KEYCODE_UNKNOWN, /* 0x56 */
            KEYCODE_UNKNOWN, /* 0x57 */
            KEYCODE_CAPSLOCK, /* 0x58 */
            KEYCODE_RIGHT_SHIFT, /* 0x59 */
            KEYCODE_ENTER, /* 0x5a */
            KEYCODE_RIGHT_BRACKET, /* 0x5b */
            KEYCODE_UNKNOWN, /* 0x5c */
            KEYCODE_BACKSLASH, /* 0x5d */
            KEYCODE_UNKNOWN, /* 0x5e */
            KEYCODE_UNKNOWN, /* 0x5f */
            KEYCODE_UNKNOWN, /* 0x60 */
            KEYCODE_UNKNOWN, /* 0x61 */
            KEYCODE_UNKNOWN, /* 0x62 */
            KEYCODE_UNKNOWN, /* 0x63 */
            KEYCODE_UNKNOWN, /* 0x64 */
            KEYCODE_UNKNOWN, /* 0x65 */
            KEYCODE_BACKSPACE, /* 0x66 */
            KEYCODE_UNKNOWN, /* 0x67 */
            KEYCODE_UNKNOWN, /* 0x68 */
            KEYCODE_KEYPAD_1, /* 0x69 */
            KEYCODE_UNKNOWN, /* 0x6a */
            KEYCODE_KEYPAD_4, /* 0x6b */
            KEYCODE_KEYPAD_7, /* 0x6c */
            KEYCODE_UNKNOWN, /* 0x6d */
            KEYCODE_UNKNOWN, /* 0x6e */
            KEYCODE_UNKNOWN, /* 0x6f */
            KEYCODE_KEYPAD_0, /* 0x70 */
            KEYCODE_KEYPAD_FULLSTOP, /* 0x71 */
            KEYCODE_KEYPAD_2, /* 0x72 */
            KEYCODE_KEYPAD_5, /* 0x73 */
            KEYCODE_KEYPAD_6, /* 0x74 */
            KEYCODE_KEYPAD_8, /* 0x75 */
            KEYCODE_ESCAPE, /* 0x76 */
            KEYCODE_NUMBER_LOCK, /* 0x77 */
            KEYCODE_F11, /* 0x78 */
            KEYCODE_KEYPAD_PLUS, /* 0x79 */
            KEYCODE_KEYPAD_3, /* 0x7a */
            KEYCODE_KEYPAD_MINUS, /* 0x7b */
            KEYCODE_KEYPAD_MULTIPLY, /* 0x7c */
            KEYCODE_KEYPAD_9, /* 0x7d */
            KEYCODE_SCROLL_LOCK, /* 0x7e */
            KEYCODE_UNKNOWN, /* 0x7f */
            KEYCODE_UNKNOWN, /* 0x80 */
            KEYCODE_UNKNOWN, /* 0x81 */
            KEYCODE_UNKNOWN, /* 0x82 */
            KEYCODE_F7, /* 0x83 */
            KEYCODE_UNKNOWN, /* 0x84 */
            KEYCODE_UNKNOWN, /* 0x85 */
            KEYCODE_UNKNOWN, /* 0x86 */
            KEYCODE_UNKNOWN, /* 0x87 */
            KEYCODE_UNKNOWN, /* 0x88 */
            KEYCODE_UNKNOWN, /* 0x89 */
            KEYCODE_UNKNOWN, /* 0x8a */
            KEYCODE_UNKNOWN, /* 0x8b */
            KEYCODE_UNKNOWN, /* 0x8c */
            KEYCODE_UNKNOWN, /* 0x8d */
            KEYCODE_UNKNOWN, /* 0x8e */
            KEYCODE_UNKNOWN, /* 0x8f */
            KEYCODE_UNKNOWN, /* 0x90 */
            KEYCODE_UNKNOWN, /* 0x91 */
            KEYCODE_UNKNOWN, /* 0x92 */
            KEYCODE_UNKNOWN, /* 0x93 */
            KEYCODE_UNKNOWN, /* 0x94 */
            KEYCODE_UNKNOWN, /* 0x95 */
            KEYCODE_UNKNOWN, /* 0x96 */
            KEYCODE_UNKNOWN, /* 0x97 */
            KEYCODE_UNKNOWN, /* 0x98 */
            KEYCODE_UNKNOWN, /* 0x99 */
            KEYCODE_UNKNOWN, /* 0x9a */
            KEYCODE_UNKNOWN, /* 0x9b */
            KEYCODE_UNKNOWN, /* 0x9c */
            KEYCODE_UNKNOWN, /* 0x9d */
            KEYCODE_UNKNOWN, /* 0x9e */
            KEYCODE_UNKNOWN, /* 0x9f */
            KEYCODE_UNKNOWN, /* 0xa0 */
            KEYCODE_UNKNOWN, /* 0xa1 */
            KEYCODE_UNKNOWN, /* 0xa2 */
            KEYCODE_UNKNOWN, /* 0xa3 */
            KEYCODE_UNKNOWN, /* 0xa4 */
            KEYCODE_UNKNOWN, /* 0xa5 */
            KEYCODE_UNKNOWN, /* 0xa6 */
            KEYCODE_UNKNOWN, /* 0xa7 */
            KEYCODE_UNKNOWN, /* 0xa8 */
            KEYCODE_UNKNOWN, /* 0xa9 */
            KEYCODE_UNKNOWN, /* 0xaa */
            KEYCODE_UNKNOWN, /* 0xab */
            KEYCODE_UNKNOWN, /* 0xac */
            KEYCODE_UNKNOWN, /* 0xad */
            KEYCODE_UNKNOWN, /* 0xae */
            KEYCODE_UNKNOWN, /* 0xaf */
            KEYCODE_UNKNOWN, /* 0xb0 */
            KEYCODE_UNKNOWN, /* 0xb1 */
            KEYCODE_UNKNOWN, /* 0xb2 */
            KEYCODE_UNKNOWN, /* 0xb3 */
            KEYCODE_UNKNOWN, /* 0xb4 */
            KEYCODE_UNKNOWN, /* 0xb5 */
            KEYCODE_UNKNOWN, /* 0xb6 */
            KEYCODE_UNKNOWN, /* 0xb7 */
            KEYCODE_UNKNOWN, /* 0xb8 */
            KEYCODE_UNKNOWN, /* 0xb9 */
            KEYCODE_UNKNOWN, /* 0xba */
            KEYCODE_UNKNOWN, /* 0xbb */
            KEYCODE_UNKNOWN, /* 0xbc */
            KEYCODE_UNKNOWN, /* 0xbd */
            KEYCODE_UNKNOWN, /* 0xbe */
            KEYCODE_UNKNOWN, /* 0xbf */
            KEYCODE_UNKNOWN, /* 0xc0 */
            KEYCODE_UNKNOWN, /* 0xc1 */
            KEYCODE_UNKNOWN, /* 0xc2 */
            KEYCODE_UNKNOWN, /* 0xc3 */
            KEYCODE_UNKNOWN, /* 0xc4 */
            KEYCODE_UNKNOWN, /* 0xc5 */
            KEYCODE_UNKNOWN, /* 0xc6 */
            KEYCODE_UNKNOWN, /* 0xc7 */
            KEYCODE_UNKNOWN, /* 0xc8 */
            KEYCODE_UNKNOWN, /* 0xc9 */
            KEYCODE_UNKNOWN, /* 0xca */
            KEYCODE_UNKNOWN, /* 0xcb */
            KEYCODE_UNKNOWN, /* 0xcc */
            KEYCODE_UNKNOWN, /* 0xcd */
            KEYCODE_UNKNOWN, /* 0xce */
            KEYCODE_UNKNOWN, /* 0xcf */
            KEYCODE_UNKNOWN, /* 0xd0 */
            KEYCODE_UNKNOWN, /* 0xd1 */
            KEYCODE_UNKNOWN, /* 0xd2 */
            KEYCODE_UNKNOWN, /* 0xd3 */
            KEYCODE_UNKNOWN, /* 0xd4 */
            KEYCODE_UNKNOWN, /* 0xd5 */
            KEYCODE_UNKNOWN, /* 0xd6 */
            KEYCODE_UNKNOWN, /* 0xd7 */
            KEYCODE_UNKNOWN, /* 0xd8 */
            KEYCODE_UNKNOWN, /* 0xd9 */
            KEYCODE_UNKNOWN, /* 0xda */
            KEYCODE_UNKNOWN, /* 0xdb */
            KEYCODE_UNKNOWN, /* 0xdc */
            KEYCODE_UNKNOWN, /* 0xdd */
            KEYCODE_UNKNOWN, /* 0xde */
            KEYCODE_UNKNOWN, /* 0xdf */
            KEYCODE_UNKNOWN, /* 0xe0 */
            KEYCODE_UNKNOWN, /* 0xe1 */
            KEYCODE_UNKNOWN, /* 0xe2 */
            KEYCODE_UNKNOWN, /* 0xe3 */
            KEYCODE_UNKNOWN, /* 0xe4 */
            KEYCODE_UNKNOWN, /* 0xe5 */
            KEYCODE_UNKNOWN, /* 0xe6 */
            KEYCODE_UNKNOWN, /* 0xe7 */
            KEYCODE_UNKNOWN, /* 0xe8 */
            KEYCODE_UNKNOWN, /* 0xe9 */
            KEYCODE_UNKNOWN, /* 0xea */
            KEYCODE_UNKNOWN, /* 0xeb */
            KEYCODE_UNKNOWN, /* 0xec */
            KEYCODE_UNKNOWN, /* 0xed */
            KEYCODE_UNKNOWN, /* 0xee */
            KEYCODE_UNKNOWN, /* 0xef */
            KEYCODE_UNKNOWN, /* 0xf0 */
            KEYCODE_UNKNOWN, /* 0xf1 */
            KEYCODE_UNKNOWN, /* 0xf2 */
            KEYCODE_UNKNOWN, /* 0xf3 */
            KEYCODE_UNKNOWN, /* 0xf4 */
            KEYCODE_UNKNOWN, /* 0xf5 */
            KEYCODE_UNKNOWN, /* 0xf6 */
            KEYCODE_UNKNOWN, /* 0xf7 */
            KEYCODE_UNKNOWN, /* 0xf8 */
            KEYCODE_UNKNOWN, /* 0xf9 */
            KEYCODE_UNKNOWN, /* 0xfa */
            KEYCODE_UNKNOWN, /* 0xfb */
            KEYCODE_UNKNOWN, /* 0xfc */
            KEYCODE_UNKNOWN, /* 0xfd */
            KEYCODE_UNKNOWN, /* 0xfe */
            KEYCODE_UNKNOWN, /* 0xff */
        };

        constexpr Keycode SCANCODE_MAP_E0[256] = {
            KEYCODE_UNKNOWN, /* 0x00 */
            KEYCODE_UNKNOWN, /* 0x01 */
            KEYCODE_UNKNOWN, /* 0x02 */
            KEYCODE_UNKNOWN, /* 0x03 */
            KEYCODE_UNKNOWN, /* 0x04 */
            KEYCODE_UNKNOWN, /* 0x05 */
            KEYCODE_UNKNOWN, /* 0x06 */
            KEYCODE_UNKNOWN, /* 0x07 */
            KEYCODE_UNKNOWN, /* 0x08 */
            KEYCODE_UNKNOWN, /* 0x09 */
            KEYCODE_UNKNOWN, /* 0x0a */
            KEYCODE_UNKNOWN, /* 0x0b */
            KEYCODE_UNKNOWN, /* 0x0c */
            KEYCODE_UNKNOWN, /* 0x0d */
            KEYCODE_UNKNOWN, /* 0x0e */
            KEYCODE_UNKNOWN, /* 0x0f */
            KEYCODE_UNKNOWN, /* 0x10 */
            KEYCODE_RIGHT_ALT, /* 0x11 */
            KEYCODE_UNKNOWN, /* 0x12 */
            KEYCODE_UNKNOWN, /* 0x13 */
            KEYCODE_RIGHT_CTRL, /* 0x14 */
            KEYCODE_UNKNOWN, /* 0x15 */
            KEYCODE_UNKNOWN, /* 0x16 */
            KEYCODE_UNKNOWN, /* 0x17 */
            KEYCODE_UNKNOWN, /* 0x18 */
            KEYCODE_UNKNOWN, /* 0x19 */
            KEYCODE_UNKNOWN, /* 0x1a */
            KEYCODE_UNKNOWN, /* 0x1b */
            KEYCODE_UNKNOWN, /* 0x1c */
            KEYCODE_UNKNOWN, /* 0x1d */
            KEYCODE_UNKNOWN, /* 0x1e */
            KEYCODE_LEFT_SUPER, /* 0x1f */
            KEYCODE_UNKNOWN, /* 0x20 */
            KEYCODE_UNKNOWN, /* 0x21 */
            KEYCODE_UNKNOWN, /* 0x22 */
            KEYCODE_UNKNOWN, /* 0x23 */
            KEYCODE_UNKNOWN, /* 0x24 */
            KEYCODE_UNKNOWN, /* 0x25 */
            KEYCODE_UNKNOWN, /* 0x26 */
            KEYCODE_RIGHT_SUPER, /* 0x27 */
            KEYCODE_UNKNOWN, /* 0x28 */
            KEYCODE_UNKNOWN, /* 0x29 */
            KEYCODE_UNKNOWN, /* 0x2a */
            KEYCODE_UNKNOWN, /* 0x2b */
            KEYCODE_UNKNOWN, /* 0x2c */
            KEYCODE_UNKNOWN, /* 0x2d */
            KEYCODE_UNKNOWN, /* 0x2e */
            KEYCODE_MENU, /* 0x2f */
            KEYCODE_UNKNOWN, /* 0x30 */
            KEYCODE_UNKNOWN, /* 0x31 */
            KEYCODE_UNKNOWN, /* 0x32 */
            KEYCODE_UNKNOWN, /* 0x33 */
            KEYCODE_UNKNOWN, /* 0x34 */
            KEYCODE_UNKNOWN, /* 0x35 */
            KEYCODE_UNKNOWN, /* 0x36 */
            KEYCODE_UNKNOWN, /* 0x37 */
            KEYCODE_UNKNOWN, /* 0x38 */
            KEYCODE_UNKNOWN, /* 0x39 */
            KEYCODE_UNKNOWN, /* 0x3a */
            KEYCODE_UNKNOWN, /* 0x3b */
            KEYCODE_UNKNOWN, /* 0x3c */
            KEYCODE_UNKNOWN, /* 0x3d */
            KEYCODE_UNKNOWN, /* 0x3e */
            KEYCODE_UNKNOWN, /* 0x3f */
            KEYCODE_UNKNOWN, /* 0x40 */
            KEYCODE_UNKNOWN, /* 0x41 */
            KEYCODE_UNKNOWN, /* 0x42 */
            KEYCODE_UNKNOWN, /* 0x43 */
            KEYCODE_UNKNOWN, /* 0x44 */
            KEYCODE_UNKNOWN, /* 0x45 */
            KEYCODE_UNKNOWN, /* 0x46 */
            KEYCODE_UNKNOWN, /* 0x47 */
            KEYCODE_UNKNOWN, /* 0x48 */
            KEYCODE_UNKNOWN, /* 0x49 */
            KEYCODE_KEYPAD_SLASH, /* 0x4a */
            KEYCODE_UNKNOWN, /* 0x4b */
            KEYCODE_UNKNOWN, /* 0x4c */
            KEYCODE_UNKNOWN, /* 0x4d */
            KEYCODE_UNKNOWN, /* 0x4e */
            KEYCODE_UNKNOWN, /* 0x4f */
            KEYCODE_UNKNOWN, /* 0x50 */
            KEYCODE_UNKNOWN, /* 0x51 */
            KEYCODE_UNKNOWN, /* 0x52 */
            KEYCODE_UNKNOWN, /* 0x53 */
            KEYCODE_UNKNOWN, /* 0x54 */
            KEYCODE_UNKNOWN, /* 0x55 */
            KEYCODE_UNKNOWN, /* 0x56 */
            KEYCODE_UNKNOWN, /* 0x57 */
            KEYCODE_UNKNOWN, /* 0x58 */
            KEYCODE_UNKNOWN, /* 0x59 */
            KEYCODE_KEYPAD_ENTER, /* 0x5a */
            KEYCODE_UNKNOWN, /* 0x5b */
            KEYCODE_UNKNOWN, /* 0x5c */
            KEYCODE_UNKNOWN, /* 0x5d */
            KEYCODE_UNKNOWN, /* 0x5e */
            KEYCODE_UNKNOWN, /* 0x5f */
            KEYCODE_UNKNOWN, /* 0x60 */
            KEYCODE_UNKNOWN, /* 0x61 */
            KEYCODE_UNKNOWN, /* 0x62 */
            KEYCODE_UNKNOWN, /* 0x63 */
            KEYCODE_UNKNOWN, /* 0x64 */
            KEYCODE_UNKNOWN, /* 0x65 */
            KEYCODE_UNKNOWN, /* 0x66 */
            KEYCODE_UNKNOWN, /* 0x67 */
            KEYCODE_UNKNOWN, /* 0x68 */
            KEYCODE_END, /* 0x69 */
            KEYCODE_UNKNOWN, /* 0x6a */
            KEYCODE_LEFT_ARROW, /* 0x6b */
            KEYCODE_HOME, /* 0x6c */
            KEYCODE_UNKNOWN, /* 0x6d */
            KEYCODE_UNKNOWN, /* 0x6e */
            KEYCODE_UNKNOWN, /* 0x6f */
            KEYCODE_INSERT, /* 0x70 */
            KEYCODE_DELETE, /* 0x71 */
            KEYCODE_DOWN_ARROW, /* 0x72 */
            KEYCODE_UNKNOWN, /* 0x73 */
            KEYCODE_RIGHT_ARROW, /* 0x74 */
            KEYCODE_UP_ARROW, /* 0x75 */
            KEYCODE_UNKNOWN, /* 0x76 */
            KEYCODE_UNKNOWN, /* 0x77 */
            KEYCODE_UNKNOWN, /* 0x78 */
            KEYCODE_UNKNOWN, /* 0x79 */
            KEYCODE_PAGE_DOWN, /* 0x7a */
            KEYCODE_UNKNOWN, /* 0x7b */
            KEYCODE_UNKNOWN, /* 0x7c */
            KEYCODE_PAGE_UP, /* 0x7d */
            KEYCODE_UNKNOWN, /* 0x7e */
            KEYCODE_UNKNOWN, /* 0x7f */
            KEYCODE_UNKNOWN, /* 0x80 */
            KEYCODE_UNKNOWN, /* 0x81 */
            KEYCODE_UNKNOWN, /* 0x82 */
            KEYCODE_UNKNOWN, /* 0x83 */
            KEYCODE_UNKNOWN, /* 0x84 */
            KEYCODE_UNKNOWN, /* 0x85 */
            KEYCODE_UNKNOWN, /* 0x86 */
            KEYCODE_UNKNOWN, /* 0x87 */
            KEYCODE_UNKNOWN, /* 0x88 */
            KEYCODE_UNKNOWN, /* 0x89 */
            KEYCODE_UNKNOWN, /* 0x8a */
            KEYCODE_UNKNOWN, /* 0x8b */
            KEYCODE_UNKNOWN, /* 0x8c */
            KEYCODE_UNKNOWN, /* 0x8d */
            KEYCODE_UNKNOWN, /* 0x8e */
            KEYCODE_UNKNOWN, /* 0x8f */
            KEYCODE_UNKNOWN, /* 0x90 */
            KEYCODE_UNKNOWN, /* 0x91 */
            KEYCODE_UNKNOWN, /* 0x92 */
            KEYCODE_UNKNOWN, /* 0x93 */
            KEYCODE_UNKNOWN, /* 0x94 */
            KEYCODE_UNKNOWN, /* 0x95 */
            KEYCODE_UNKNOWN, /* 0x96 */
            KEYCODE_UNKNOWN, /* 0x97 */
            KEYCODE_UNKNOWN, /* 0x98 */
            KEYCODE_UNKNOWN, /* 0x99 */
            KEYCODE_UNKNOWN, /* 0x9a */
            KEYCODE_UNKNOWN, /* 0x9b */
            KEYCODE_UNKNOWN, /* 0x9c */
            KEYCODE_UNKNOWN, /* 0x9d */
            KEYCODE_UNKNOWN, /* 0x9e */
            KEYCODE_UNKNOWN, /* 0x9f */
            KEYCODE_UNKNOWN, /* 0xa0 */
            KEYCODE_UNKNOWN, /* 0xa1 */
            KEYCODE_UNKNOWN, /* 0xa2 */
            KEYCODE_UNKNOWN, /* 0xa3 */
            KEYCODE_UNKNOWN, /* 0xa4 */
            KEYCODE_UNKNOWN, /* 0xa5 */
            KEYCODE_UNKNOWN, /* 0xa6 */
            KEYCODE_UNKNOWN, /* 0xa7 */
            KEYCODE_UNKNOWN, /* 0xa8 */
            KEYCODE_UNKNOWN, /* 0xa9 */
            KEYCODE_UNKNOWN, /* 0xaa */
            KEYCODE_UNKNOWN, /* 0xab */
            KEYCODE_UNKNOWN, /* 0xac */
            KEYCODE_UNKNOWN, /* 0xad */
            KEYCODE_UNKNOWN, /* 0xae */
            KEYCODE_UNKNOWN, /* 0xaf */
            KEYCODE_UNKNOWN, /* 0xb0 */
            KEYCODE_UNKNOWN, /* 0xb1 */
            KEYCODE_UNKNOWN, /* 0xb2 */
            KEYCODE_UNKNOWN, /* 0xb3 */
            KEYCODE_UNKNOWN, /* 0xb4 */
            KEYCODE_UNKNOWN, /* 0xb5 */
            KEYCODE_UNKNOWN, /* 0xb6 */
            KEYCODE_UNKNOWN, /* 0xb7 */
            KEYCODE_UNKNOWN, /* 0xb8 */
            KEYCODE_UNKNOWN, /* 0xb9 */
            KEYCODE_UNKNOWN, /* 0xba */
            KEYCODE_UNKNOWN, /* 0xbb */
            KEYCODE_UNKNOWN, /* 0xbc */
            KEYCODE_UNKNOWN, /* 0xbd */
            KEYCODE_UNKNOWN, /* 0xbe */
            KEYCODE_UNKNOWN, /* 0xbf */
            KEYCODE_UNKNOWN, /* 0xc0 */
            KEYCODE_UNKNOWN, /* 0xc1 */
            KEYCODE_UNKNOWN, /* 0xc2 */
            KEYCODE_UNKNOWN, /* 0xc3 */
            KEYCODE_UNKNOWN, /* 0xc4 */
            KEYCODE_UNKNOWN, /* 0xc5 */
            KEYCODE_UNKNOWN, /* 0xc6 */
            KEYCODE_UNKNOWN, /* 0xc7 */
            KEYCODE_UNKNOWN, /* 0xc8 */
            KEYCODE_UNKNOWN, /* 0xc9 */
            KEYCODE_UNKNOWN, /* 0xca */
            KEYCODE_UNKNOWN, /* 0xcb */
            KEYCODE_UNKNOWN, /* 0xcc */
            KEYCODE_UNKNOWN, /* 0xcd */
            KEYCODE_UNKNOWN, /* 0xce */
            KEYCODE_UNKNOWN, /* 0xcf */
            KEYCODE_UNKNOWN, /* 0xd0 */
            KEYCODE_UNKNOWN, /* 0xd1 */
            KEYCODE_UNKNOWN, /* 0xd2 */
            KEYCODE_UNKNOWN, /* 0xd3 */
            KEYCODE_UNKNOWN, /* 0xd4 */
            KEYCODE_UNKNOWN, /* 0xd5 */
            KEYCODE_UNKNOWN, /* 0xd6 */
            KEYCODE_UNKNOWN, /* 0xd7 */
            KEYCODE_UNKNOWN, /* 0xd8 */
            KEYCODE_UNKNOWN, /* 0xd9 */
            KEYCODE_UNKNOWN, /* 0xda */
            KEYCODE_UNKNOWN, /* 0xdb */
            KEYCODE_UNKNOWN, /* 0xdc */
            KEYCODE_UNKNOWN, /* 0xdd */
            KEYCODE_UNKNOWN, /* 0xde */
            KEYCODE_UNKNOWN, /* 0xdf */
            KEYCODE_UNKNOWN, /* 0xe0 */
            KEYCODE_UNKNOWN, /* 0xe1 */
            KEYCODE_UNKNOWN, /* 0xe2 */
            KEYCODE_UNKNOWN, /* 0xe3 */
            KEYCODE_UNKNOWN, /* 0xe4 */
            KEYCODE_UNKNOWN, /* 0xe5 */
            KEYCODE_UNKNOWN, /* 0xe6 */
            KEYCODE_UNKNOWN, /* 0xe7 */
            KEYCODE_UNKNOWN, /* 0xe8 */
            KEYCODE_UNKNOWN, /* 0xe9 */
            KEYCODE_UNKNOWN, /* 0xea */
            KEYCODE_UNKNOWN, /* 0xeb */
            KEYCODE_UNKNOWN, /* 0xec */
            KEYCODE_UNKNOWN, /* 0xed */
            KEYCODE_UNKNOWN, /* 0xee */
            KEYCODE_UNKNOWN, /* 0xef */
            KEYCODE_UNKNOWN, /* 0xf0 */
            KEYCODE_UNKNOWN, /* 0xf1 */
            KEYCODE_UNKNOWN, /* 0xf2 */
            KEYCODE_UNKNOWN, /* 0xf3 */
            KEYCODE_UNKNOWN, /* 0xf4 */
            KEYCODE_UNKNOWN, /* 0xf5 */
            KEYCODE_UNKNOWN, /* 0xf6 */
            KEYCODE_UNKNOWN, /* 0xf7 */
            KEYCODE_UNKNOWN, /* 0xf8 */
            KEYCODE_UNKNOWN, /* 0xf9 */
            KEYCODE_UNKNOWN, /* 0xfa */
            KEYCODE_UNKNOWN, /* 0xfb */
            KEYCODE_UNKNOWN, /* 0xfc */
            KEYCODE_UNKNOWN, /* 0xfd */
            KEYCODE_UNKNOWN, /* 0xfe */
            KEYCODE_UNKNOWN, /* 0xff */
        };

        enum class ParseState {
            BEGIN,
            E0,
            E012
        };

        const auto get_response_unsafe = []() -> u8 {
            for (size_t i = 0; i < 3; i++) {
                if (port_read_byte(0x64) & 0x01) {
                    return port_read_byte(0x60);
                }
                io_wait();
            }
            return 0x00;
        };

        const u8 b1 = get_response_unsafe();
        if (b1 == 0xE0) {
            const u8 b2 = get_response_unsafe();

            // unimplemented
            if (b2 == 0x12) {
                while (get_response_unsafe() != 0x00) {}
                r_event = KeyEvent::PRESSED;
                return KEYCODE_UNKNOWN;
            }
            else if (b2 == 0xF0) {
                const u8 b3 = get_response_unsafe();

                // unimplemented
                if (b2 == 0x12) {
                    while (get_response_unsafe() != 0x00) {}
                    r_event = KeyEvent::PRESSED;
                    return KEYCODE_UNKNOWN;
                }
                else {
                    r_event = KeyEvent::RELEASED;
                    return SCANCODE_MAP_E0[b3];
                }
            }
            else {
                r_event = KeyEvent::PRESSED;
                return SCANCODE_MAP_E0[b2];
            }
        }
        else if (b1 == 0xF0) {
            const u8 b2 = get_response_unsafe();
            r_event = KeyEvent::RELEASED;
            return SCANCODE_MAP_SINGLE_BYTE[b2];
        }
        else {
            r_event = KeyEvent::PRESSED;
            return SCANCODE_MAP_SINGLE_BYTE[b1];
        }
        
    }

    bool is_key_pressed(Keycode t_key) {
        return KEYBOARD_KEY_STATE[t_key];
    }

    INTERRUPT_HANDLER void keyboard_handler(InterruptHandler::InterruptFrame* t_frame) {
        KeyEvent event = KeyEvent::PRESSED;
        const Keycode keycode = parse_scancode(event);

        if (keycode != KEYCODE_UNKNOWN) {
            KEYBOARD_EVENT_QUEUE.push_back({keycode, event});
            KEYBOARD_KEY_STATE[keycode] = (event == KeyEvent::PRESSED);
        }

        // Acknowledge interrupt
        PIC::send_end_of_interrupt(0x21);
    }

}
