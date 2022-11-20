#ifndef GDT_INCLUDED
#define GDT_INCLUDED

#include "common.hpp"

namespace Kernel::GDT {

    void initialize();

    int add_entry(u16, u16, u16, u16);

    void load_table();

}

#endif
