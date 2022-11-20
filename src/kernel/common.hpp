#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <stddef.h>
#include <stdint.h>

#include "error.hpp"

namespace Kernel {

    using s8 = int8_t;
    using u8 = uint8_t;
    using s16 = int16_t;
    using u16 = uint16_t;
    using s32 = int32_t;
    using u32 = uint32_t;
    using s64 = int64_t;
    using u64 = uint64_t;

    u8 port_read_byte(u16 t_port);
    u16 port_read_hword(u16 t_port);
    void port_write_byte(u16 t_port, u8 t_value);

    u8 read_cmos(u8 t_register);

    void io_wait();

    // sleep(1000) sleeps for 1 second
    void sleep(u32 t_milliseconds);

    void enable_interrupts();
    void disable_interrupts();

    template <typename T>
    constexpr T get_smallest_gte_multiple(T t_value, T t_multiple) {
        if (t_value % t_multiple == 0) {
            return t_value;
        }
        else {
            return t_value + (t_multiple - (t_value % t_multiple));
        }
    }

}

extern "C" void* memcpy(void* t_dest, const void* t_src, size_t t_count);
extern "C" void* memmove(void* t_dest, const void* t_src, size_t t_count);
extern "C" void* memset(void* t_dest, int t_ch, size_t t_count);
extern "C" int memcmp(const void* t_lhs, const void* t_rhs, size_t t_count);
	

#define KERNEL_HALT() asm("hlt")
#define KERNEL_STOP() do {\
    disable_interrupts();\
    KERNEL_HALT();\
} while(false)

#define GOT_HERE() do {\
    VGA::put_string("GOT HERE: '" __FILE__ "' (");\
    VGA::put_unsigned_decimal(__LINE__);\
    VGA::put_string(")\n");\
} while(false)

#endif
