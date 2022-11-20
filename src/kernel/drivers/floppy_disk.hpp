#ifndef FLOPPY_DISK_INCLUDED
#define FLOPPY_DISK_INCLUDED

#include "common.hpp"
#include "data/error_or.hpp"
#include "interrupts/interrupt_handler.hpp"

namespace Kernel::FloppyDisk {

    // TODO: move this into floppy_disk.cpp
    constexpr size_t SECTOR_SIZE = 512;
    
    Data::ErrorOr<void> initialize();
    Data::ErrorOr<void> reset();

    Data::ErrorOr<void> read_data(u8 t_drive, size_t t_lba, size_t t_count, u8* r_buffer);

    INTERRUPT_HANDLER void floppy_handler(InterruptHandler::InterruptFrame* t_frame);
}

#endif
