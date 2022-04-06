#ifndef IDT_INCLUDED
#define IDT_INCLUDED

#include "common.hpp"

namespace Kernel::IDT {

    enum class IDTGateType {
        TASK,
        INTERRUPT,
        TRAP
    };

    void initialize();

    int add_entry(void* t_handlerAddress, u16 t_segmentSelector, IDTGateType t_gateType, bool t_32bit);

    void load_table();

}

#endif
