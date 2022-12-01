#include "common.hpp"
#include "drivers/pit/pit.hpp"

namespace Kernel {

    u8 port_read_byte(u16 t_port) {
        u8 b = 0;
        asm volatile (
            "inb %1, %0"
            : "=r" (b)
            : "dN" (t_port)
        );
        return b;
    }

    u16 port_read_hword(u16 t_port) {
        u16 hw = 0;
        asm volatile (
            "inw %1, %0"
            : "=r" (hw)
            : "dN" (t_port)
        );
        return hw;
    }

    void port_write_byte(u16 t_port, u8 t_value) {
        asm volatile (
            "outb %0, %1"
            :
            : "a" (t_value), "dN" (t_port)
        );
    }

    u8 read_cmos(u8 t_register) {
        port_write_byte(0x70, t_register);
        io_wait();
        return port_read_byte(0x71);
    }

    void io_wait() {
        port_write_byte(0x80, 0);
    }

    void sleep(uint t_ticks) {
        const uint start = PIT::get_ticks();

        while (PIT::get_ticks() - start < t_ticks) {
            KERNEL_HALT();
        }
    }

    void enable_interrupts() {
        asm volatile ("sti");
    }

    void disable_interrupts() {
        asm volatile ("cli");
    }

    
}

// TODO: test all of these!
void* memcpy(void* t_dest, const void* t_src, size_t t_count) {
    for (size_t i = 0; i < t_count; i++) {
        *(reinterpret_cast<unsigned char*>(t_dest) + i) = *(reinterpret_cast<const unsigned char*>(t_src) + i);
    }

    return t_dest;
}

void* memmove(void* t_dest, const void* t_src, size_t t_count) {
    if (t_dest < t_src) {
        return memcpy(t_dest, t_src, t_count);
    }
    else if (t_dest > t_src) {
        for (size_t i = 0; i < t_count; i++) {
            const size_t offset = t_count - 1 - i;

            *(reinterpret_cast<unsigned char*>(t_dest) + offset) = *(reinterpret_cast<const unsigned char*>(t_src) + offset);
        }
    }
    return t_dest;
}

void* memset(void* t_dest, int t_ch, size_t t_count) {
    for (size_t i = 0; i < t_count; i++) {
        *(reinterpret_cast<unsigned char*>(t_dest) + i) = static_cast<unsigned char>(t_ch);
    }
    return t_dest;
}

int memcmp(const void* t_lhs, const void* t_rhs, size_t t_count) {
    for (size_t i = 0; i < t_count; i++) {
        const unsigned char leftByte = *(reinterpret_cast<const unsigned char*>(t_lhs) + i);
        const unsigned char rightByte = *(reinterpret_cast<const unsigned char*>(t_rhs) + i);

        if (leftByte < rightByte) {
            return -1;
        }
        else if (leftByte > rightByte) {
            return 1;
        }
    }

    return 0;
}

