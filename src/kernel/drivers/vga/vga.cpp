#include "common.hpp"
#include "vga.hpp"

namespace Kernel::VGA {

    constexpr size_t TTY_WIDTH = 80;
    constexpr size_t TTY_HEIGHT = 25;

    u16* const TEXT_BUFFER = (u16*) 0xB8000;

    static CursorPos cursor = { 0, 0 };

    void initialize() {
        clear_screen();
    }

    void clear_screen() {
        for (u32 i = 0; i < TTY_WIDTH * TTY_HEIGHT; i++) {
            TEXT_BUFFER[i] = 0x0700;
        }
        set_cursor_pos(0, 0);
    }

    void put_char(char t_c) {
        TEXT_BUFFER[cursor.y * TTY_WIDTH + cursor.x] = 0x0700 | t_c;
        const u8 newX = (cursor.x + 1) % TTY_WIDTH;
        const u8 newY = (newX == 0) ? (cursor.y + 1) : (cursor.y);
        set_cursor_pos(newX, newY);
    }

    void put_string(const char* t_string) {
        for (const char* p = t_string; *p != '\0'; p++) {
            const char c = *p;
            switch (c) {
                case '\n':
                    new_line();
                    break;
                case '\b':
                    if (cursor.x > 0) {
                        set_cursor_pos(cursor.x - 1, cursor.y);
                    }
                    break;
                case '\r':
                    set_cursor_pos(0, cursor.y);
                    break;
                default:
                    put_char(c);
            }
        }
    }

    void put_hex(u32 t_value) {
        put_string("0x");
        for (u32 i = 0; i < 2 * sizeof(u32); i++) {
            const u8 hexDigit = t_value >> 28;
            if (hexDigit < 10) {
                put_char(hexDigit + '0');
            }
            else {
                put_char((hexDigit - 10) + 'A');
            }
            t_value = t_value << 4;
        }
    }

    void put_signed_decimal(s32 t_value) {
        if (t_value < 0) {
            put_char('-');
            t_value = -t_value;
        }
        put_unsigned_decimal(t_value);
    }

    void put_unsigned_decimal(u32 t_value) {
        if (t_value == 0) {
            put_char('0');
            return;
        }

        constexpr s32 MAX_DIGITS = 10; // UINT_MAX = 2^32 - 1 = 4294967295 which has 10 digits
        char DIGIT_BUFFER[MAX_DIGITS] = {0}; 

        // Calculate digits and write them to DIGIT_BUFFER (LSD first)
        s32 i = 0;
        for (; i < MAX_DIGITS && t_value != 0; i++) {
            const u8 digit = t_value % 10;
            t_value /= 10;
            DIGIT_BUFFER[i] = digit + '0';
        }

        // Print digits in reverse order (so that 123 get printed as 123 and not 321)
        const s32 noDigits = i;
        for (s32 i = noDigits - 1; i >= 0; i--) {
            put_char(DIGIT_BUFFER[i]);
        }

    }

    void new_line() {
        set_cursor_pos(0, cursor.y + 1);
    }

    CursorPos get_cursor_pos() {
        return cursor;
    }

    void set_cursor_pos(u8 t_x, u8 t_y) {
        cursor.x = t_x;
        cursor.y = t_y;

        for (; cursor.y >= TTY_HEIGHT; cursor.y--) {
            for (u16 y = 0; y < TTY_HEIGHT - 1; y++) {
                for (u16 x = 0; x < TTY_WIDTH; x++) {
                    TEXT_BUFFER[y * TTY_WIDTH + x] = TEXT_BUFFER[(y + 1) * TTY_WIDTH + x];
                }
            }
            for (u8 x = 0; x < TTY_WIDTH; x++) {
                TEXT_BUFFER[(TTY_HEIGHT - 1) * TTY_WIDTH + x] = ' ';
            }
        }

        const u16 cursorPos = cursor.y * TTY_WIDTH + cursor.x;

        port_write_byte(0x3D4, 14); // tell VGA board that we are setting the high cursor byte
        port_write_byte(0x3D5, cursorPos >> 8);
        port_write_byte(0x3D4, 15); // tell VGA that we are setting the low cursor byte
        port_write_byte(0x3D5, cursorPos & 0xFF);
    }

    void offset_cursor(u8 t_dx, u8 t_dy) {
        set_cursor_pos(cursor.x + t_dx, cursor.y + t_dy);
    }

}
