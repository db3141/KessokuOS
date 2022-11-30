#ifndef GDT_INCLUDED
#define GDT_INCLUDED

#include "common.hpp"
#include "data/error_or.hpp"

namespace Kernel::GDT {

    void initialize();

    Data::ErrorOr<void> add_entry(u16, u16, u16, u16);

    void load_table();

}

#endif
