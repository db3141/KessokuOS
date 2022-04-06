#ifndef FLOPPY_DISK_INCLUDED
#define FLOPPY_DISK_INCLUDED

#include "common.hpp"
#include "sznnlib/error_or.hpp"

namespace Kernel::FloppyDisk {

    enum Error : int {
        ERROR_CONTROLLER_NOT_EXPECTING_WRITE = ErrorCodeGroup::DRIVERS_FLOPPY_DISK,
        ERROR_SEND_TIMEOUT,
        ERROR_UNSUPPORTED_VERSION,
        ERROR_LOCK_FAILED,
        ERROR_INVALID_DRIVE,
    };
    
    SZNN::ErrorOr<void> initialize();

    INTERRUPT_HANDLER void floppy_handler(InterruptHandler::InterruptFrame* t_frame);
}

#endif
