#include "gdt.hpp"

namespace Kernel::GDT {

    struct GDTEntry {
        u16 w1, w2, w3, w4;
    };

    struct GDT {
        u16 _; // unused
        u16 size;
        GDTEntry* entries;
    };

    constexpr size_t GDT_MAX_ENTRY_COUNT = 5;

    static GDTEntry GDT_ENTRIES[GDT_MAX_ENTRY_COUNT];
    static GDT KERNEL_GDT;

    void initialize() {
        KERNEL_GDT.entries = &GDT_ENTRIES[0];
        KERNEL_GDT.size = 0;
    }

    int add_entry(u16 t_w1, u16 t_w2, u16 t_w3, u16 t_w4) {
        const size_t entryCount = KERNEL_GDT.size / sizeof(GDTEntry);

        if (entryCount >= GDT_MAX_ENTRY_COUNT) {
            return 1;
        }
        const GDTEntry entry = { t_w1, t_w2, t_w3, t_w4 };
        KERNEL_GDT.entries[entryCount] = entry;
        KERNEL_GDT.size += sizeof(GDTEntry);

        return 0;
    }

    void load_table() {
        KERNEL_GDT.size--; // size should contain size of all entries - 1

        u8* gdtPointer = ((u8*) &KERNEL_GDT) + 2; // ignore unused bytes in struct
        asm volatile ("lgdt (%0)" : : "r" (gdtPointer) );
    }

}
