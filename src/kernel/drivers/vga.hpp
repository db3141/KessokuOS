#ifndef VGA_INCLUDED
#define VGA_INCLUDED

#include "common.hpp"

namespace Kernel::VGA {

    struct CursorPos {
        u8 x, y;
    };

    void initialize();

    void clear_screen();

    void put_char(char t_c);
    void put_string(const char* t_string);

    void put_hex(u32 t_value);
    void put_signed_decimal(s32 t_value);
    void put_unsigned_decimal(u32 t_value);
    
    void new_line();

    CursorPos get_cursor_pos();
    void set_cursor_pos(u8 t_x, u8 t_y);
    void offset_cursor(u8 t_dx, u8 t_dy);

}

#endif
