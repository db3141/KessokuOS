#ifndef PIC_INCLUDED
#define PIC_INCLUDED

#include "common.hpp"

namespace Kernel::PIC {

    void initialize();

    void send_end_of_interrupt(u8 t_irq);

}

#endif
