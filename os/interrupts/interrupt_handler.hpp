#ifndef INTERRUPT_HANDLER_INCLUDED
#define INTERRUPT_HANDLER_INCLUDED

#include "common.hpp"

#define INTERRUPT_HANDLER __attribute__((interrupt))

namespace Kernel::InterruptHandler {

    struct InterruptFrame {
        u32 ip;
        u32 cs;
        u32 flags;
        u32 sp;
        u32 ss;
    };

    INTERRUPT_HANDLER void interrupt_handler(InterruptFrame* t_frame);

}

#endif
