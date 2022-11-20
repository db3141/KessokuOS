#include "pic.hpp"

namespace Kernel::PIC {

    // Thanks OSDev wiki
    // https://wiki.osdev.org/PIC#Programming_the_PIC_chips

    constexpr u8 PIC1 = 0x20; // IO base address for master PIC
    constexpr u8 PIC2 = 0xA0; // IO base address for slave PIC
    constexpr u8 PIC1_COMMAND = PIC1;
    constexpr u8 PIC2_COMMAND = PIC2;
    constexpr u8 PIC1_DATA = PIC1 + 1;
    constexpr u8 PIC2_DATA = PIC2 + 1;

    constexpr u8 PIC_EOI = 0x20;

    constexpr u8 ICW1_ICW4      = 0x01; // ICW4 (not) needed
    constexpr u8 ICW1_SINGLE    = 0x02; // Single cascade mode
    constexpr u8 ICW1_INTERVAL4 = 0x04; // Call address interval 4 (8)
    constexpr u8 ICW1_LEVEL     = 0x08; // Level triggered (edge) mode
    constexpr u8 ICW1_INIT      = 0x10; // Initialization - required!

    constexpr u8 ICW4_8086       = 0x01; // 8086/88 (MCS-80/85) mode
    constexpr u8 ICW4_AUTO       = 0x02; // Auto (normal) EOI
    constexpr u8 ICW4_BUF_SLAVE  = 0x08; // Buffered mode/slave
    constexpr u8 ICW4_BUF_MASTER = 0x0C; // Buffered mode/master
    constexpr u8 ICW4_SFNM       = 0x10; // Special fully nested (not)

    constexpr u8 PIC1_VECTOR_OFFSET = 0x20; // Set offset to start of valid range
    constexpr u8 PIC2_VECTOR_OFFSET = 0x28;

    void initialize() {
        const char a1 = port_read_byte(PIC1_DATA); // Save masks
        const char a2 = port_read_byte(PIC2_DATA);

        // starts initialization in cascade mode
        port_write_byte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
        io_wait();
        port_write_byte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        io_wait();

        // Set PIC vector offsets
        port_write_byte(PIC1_DATA, PIC1_VECTOR_OFFSET);
        io_wait();
        port_write_byte(PIC2_DATA, PIC2_VECTOR_OFFSET);
        io_wait();

        //
        port_write_byte(PIC1_DATA, 4); // tell master PIC that there is a slave PIC at IRQ2 (0000 0100)
        io_wait();
        port_write_byte(PIC2_DATA, 2); // tell slave PIC its cascade identity
        io_wait();

        port_write_byte(PIC1_DATA, ICW4_8086);
        io_wait();
        port_write_byte(PIC2_DATA, ICW4_8086);
        io_wait();

        port_write_byte(PIC1_DATA, a1); // restore saved masks
        port_write_byte(PIC2_DATA, a2);
    }

    void send_end_of_interrupt(u8 t_irq) {
        if (t_irq >= 8) {
            port_write_byte(PIC2_COMMAND, PIC_EOI);
        }
        port_write_byte(PIC1_COMMAND, PIC_EOI);
    }

}
