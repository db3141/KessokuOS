#ifndef FLOPPY_DISK_INCLUDED
#define FLOPPY_DISK_INCLUDED

#include "common.hpp"
#include "error_code_groups.hpp"
#include "interrupts/interrupt_handler.hpp"
#include "data/error_or.hpp"

namespace Kernel::FloppyDisk {

    enum Error : int {
        ERROR_CONTROLLER_NOT_EXPECTING_WRITE = ErrorCodeGroup::get_id(ErrorCodeGroup::Group::DRIVERS_FLOPPY_DISK),
        ERROR_CONTROLLER_NEEDS_RESET,
        ERROR_TIMEOUT,
        ERROR_UNSUPPORTED_VERSION,
        ERROR_LOCK_FAILED,
        ERROR_INVALID_DRIVE,
        ERROR_INVALID_PARAMETER,
        ERROR_COMMAND_FAILED
    };

    constexpr size_t SECTOR_SIZE = 512;
    
    Data::ErrorOr<void> initialize();
    Data::ErrorOr<void> reset();

    Data::ErrorOr<void> read_data(u8 t_drive, size_t t_lba, size_t t_count, u8* r_buffer);

    INTERRUPT_HANDLER void floppy_handler(InterruptHandler::InterruptFrame* t_frame);
}

#endif
