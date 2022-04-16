#ifndef FLOPPY_DISK_INCLUDED
#define FLOPPY_DISK_INCLUDED

#include "common.hpp"
#include "error_code_groups.hpp"
#include "interrupts/interrupt_handler.hpp"
#include "sznnlib/error_or.hpp"

namespace Kernel::FloppyDisk {

    enum Error : int {
        ERROR_CONTROLLER_NOT_EXPECTING_WRITE = ErrorCodeGroup::DRIVERS_FLOPPY_DISK,
        ERROR_CONTROLLER_NEEDS_RESET,
        ERROR_TIMEOUT,
        ERROR_UNSUPPORTED_VERSION,
        ERROR_LOCK_FAILED,
        ERROR_INVALID_DRIVE,
    };
    
    SZNN::ErrorOr<void> initialize();
    SZNN::ErrorOr<void> reset();

    SZNN::ErrorOr<void> read_data(u8 t_drive, u32 t_address, u32 t_count);

    INTERRUPT_HANDLER void floppy_handler(InterruptHandler::InterruptFrame* t_frame);
}

#endif
