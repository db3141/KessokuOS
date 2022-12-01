#include "interrupt_handler.hpp"
#include "pic.hpp"

namespace Kernel::InterruptHandler {

    INTERRUPT_HANDLER void interrupt_handler(InterruptFrame* t_frame) {
        // Acknowledge interrupt
        PIC::send_end_of_interrupt(0);
    }

}
