#ifndef KERNEL_DMA_INCLUDED
#define KERNEL_DMA_INCLUDED

#include "common.hpp"
#include "error_code_groups.hpp"
#include "sznnlib/error_or.hpp"

namespace Kernel::DMA {

    enum Error : int {
        ERROR_INVALID_CHANNEL = ErrorCodeGroup::get_id(ErrorCodeGroup::Group::DRIVERS_DMA)
    };

    SZNN::ErrorOr<void> initialize_channel(u8 t_channel, void* t_bufferAddress, u16 t_count);
    SZNN::ErrorOr<void> set_mode(u8 t_channel, u8 t_transferType, bool t_autoInit, bool t_down, u8 t_mode);

}

#endif
