#include "idt.hpp"

namespace Kernel::IDT {

    struct IDTEntry {
        u16 offsetLo;
        u16 segmentSelector;
        u8 _; // reserved
        u8 misc;
        u16 offsetHi;
    };

    struct IDT {
        u16 _; // unused
        u16 size;
        IDTEntry* entries;
    };

    IDTEntry idt_entry_create(u32 t_offset, u16 t_segmentSelector, IDTGateType t_gateType, bool t_32bit);

    constexpr u8 generate_misc_byte(IDTGateType t_gateType, bool t_32bit) {
        switch (t_gateType) {
            case IDTGateType::TASK:
                return 0b10000101;
            case IDTGateType::INTERRUPT:
                if (!t_32bit) {
                    return 0b10000110; // 16-bit
                }
                else {
                    return 0b10001110; // 32-bit
                }
            case IDTGateType::TRAP:
                if (!t_32bit) {
                    return 0b10000111; // 16-bit
                }
                else {
                    return 0b10001111; // 32-bit
                }
        }
    }

    constexpr size_t IDT_MAX_ENTRY_COUNT = 256;

    static IDTEntry IDT_ENTRIES[IDT_MAX_ENTRY_COUNT];
    static IDT KERNEL_IDT;

    void initialize() {
        KERNEL_IDT.size = 0;
        KERNEL_IDT.entries = IDT_ENTRIES;
    }

    IDTEntry idt_entry_create(u32 t_offset, u16 t_segmentSelector, IDTGateType t_gateType, bool t_32bit) {
        IDTEntry result = { 0, 0, 0, 0, 0 };

        if (t_gateType != IDTGateType::TASK) {
            result.offsetHi = static_cast<u16>((t_offset >> 16) & 0xFFFFFFFF);
            result.offsetLo = static_cast<u16>((t_offset >>  0) & 0xFFFFFFFF);
        }
        result.misc = generate_misc_byte(t_gateType, t_32bit);
        result.segmentSelector = t_segmentSelector;

        return result;
    }

    int add_entry(void* t_handlerAddress, u16 t_segmentSelector, IDTGateType t_gateType, bool t_32bit) {
        const u32 entryCount = KERNEL_IDT.size / sizeof(IDTEntry);
        if (entryCount >= IDT_MAX_ENTRY_COUNT) {
            return 1;
        }

        const IDTEntry entry = idt_entry_create(reinterpret_cast<u32>(t_handlerAddress), t_segmentSelector, t_gateType, t_32bit);
        KERNEL_IDT.entries[entryCount] = entry;
        KERNEL_IDT.size += sizeof(IDTEntry);

        return 0;
    }

    void load_table() {
        KERNEL_IDT.size -= 1; // size should contain size of all entries - 1

        u8* idtPointer = ((u8*) &KERNEL_IDT) + 2; // ignore unused bytes in struct
        asm volatile ("lidt (%0)" : : "r" (idtPointer));
    }

}
