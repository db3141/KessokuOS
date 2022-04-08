#include "drivers/vga.hpp"
#include "floppy_disk.hpp"
#include "interrupts/pic.hpp"
#include "sznnlib/error_or.hpp"

namespace Kernel::FloppyDisk {

    enum Register {
        STATUS_REGISTER_A                = 0x3F0, // read-only
        STATUS_REGISTER_B                = 0x3F1, // read-only
        DIGITAL_OUTPUT_REGISTER          = 0x3F2,
        TAPE_DRIVE_REGISTER              = 0x3F3,
        MAIN_STATUS_REGISTER             = 0x3F4, // read-only
        DATARATE_SELECT_REGISTER         = 0x3F4, // write-only
        DATA_FIFO                        = 0x3F5,
        DIGITAL_INPUT_REGISTER           = 0x3F7, // read-only
        CONFIGURATION_CONTROL_REGISTER   = 0x3F7  // write-only
    };

    // osdev wiki https://wiki.osdev.org/Floppy_disk
    enum Command {
        COMMAND_READ_TRACK          = 2,    // generates IRQ6
        COMMAND_SPECIFY             = 3,    // * set drive parameters
        COMMAND_SENSE_DRIVE_STATUS  = 4,
        COMMAND_WRITE_DATA          = 5,    // * write to the disk
        COMMAND_READ_DATA           = 6,    // * read from the disk
        COMMAND_RECALIBRATE         = 7,    // * seek to cylinder 0
        COMMAND_SENSE_INTERRUPT     = 8,    // * ack IRQ6, get status of last command
        COMMAND_WRITE_DELETED_DATA  = 9,
        COMMAND_READ_ID             = 10,   // generates IRQ6
        COMMAND_READ_DELETED_DATA   = 12,
        COMMAND_FORMAT_TRACK        = 13,   // *
        COMMAND_DUMPREG             = 14,
        COMMAND_SEEK                = 15,   // * seek both heads to cylinder X
        COMMAND_VERSION             = 16,   // * used during initialization, once
        COMMAND_SCAN_EQUAL          = 17,
        COMMAND_PERPENDICULAR_MODE  = 18,   // * used during initialization, once, maybe
        COMMAND_CONFIGURE           = 19,   // * set controller parameters
        COMMAND_LOCK                = 20,   // * protect controller params from a reset
        COMMAND_VERIFY              = 22,
        COMMAND_SCAN_LOW_OR_EQUAL   = 25,
        COMMAND_SCAN_HIGH_OR_EQUAL  = 29
    };

    enum class IRQType {
        RESET,
        RECALIBRATE,
        NONE
    };

    static volatile struct {
        IRQType irqType;
    } s_floppyState;

    constexpr size_t RETRY_COUNT = 10;

    SZNN::ErrorOr<u8> command_version();
    SZNN::ErrorOr<void> command_configure(u8 t_parameter);
    SZNN::ErrorOr<void> command_lock();
    SZNN::ErrorOr<void> command_specify(u8 t_p1, u8 t_p2);
    SZNN::ErrorOr<void> command_recalibrate(u8 t_drive);

    SZNN::ErrorOr<void> select_drive(u8 t_drive);

    SZNN::ErrorOr<u8> read_msr_until_rqm();

    SZNN::ErrorOr<void> initialize() {
        s_floppyState.irqType = IRQType::NONE;

        const auto result = command_version();
        if (!result.is_error()) {
            ASSERT(result.get_value() == 0x90, ERROR_UNSUPPORTED_VERSION); // Verify that the version number is 0x90
        }
        else if (result.get_error() == ERROR_CONTROLLER_NEEDS_RESET) {
            reset();
            ASSERT(TRY(command_version()) == 0x90, ERROR_UNSUPPORTED_VERSION);
        }

        TRY(command_configure(0x57)); // Implied seek on, FIFO on, drive polling mode off, threshold = 8 (7 + 1)
        TRY(command_lock());
        TRY(reset());
        TRY(command_recalibrate(0));

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> reset() {
        s_floppyState.irqType = IRQType::RESET; // Set state to be ready for a reset IRQ
        port_write_byte(DATARATE_SELECT_REGISTER, 0x80);

        // Wait for IRQ, TODO: add timeout here
        while (s_floppyState.irqType == IRQType::RESET) {
            KERNEL_HALT();
        }

        TRY(select_drive(0)); // TODO: support more than one drive

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<u8> command_version() {
        // Send command byte
        u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        ASSERT((msr & 0xc0) == 0x80, ERROR_CONTROLLER_NEEDS_RESET); // Check that RQM = 1 and DIO = 0

        port_write_byte(DATA_FIFO, COMMAND_VERSION);

        // Loop until result byte is ready
        msr = TRY(read_msr_until_rqm());
        ASSERT((msr & 0xc0) == 0xc0, int(-1)); // Check that RQM = 1 and DIO = 1 (TODO: same as above)

        // Read result byte
        const u8 result = port_read_byte(DATA_FIFO);
        msr = TRY(read_msr_until_rqm());
        ASSERT((msr & 0xe0) == 0x80, int(-1)); // Check that CMD BSY = 0 and DIO = 0

        // TODO: retry if fails here

        return result;
    }

    SZNN::ErrorOr<void> command_configure(u8 t_parameter) {
        // Send command byte
        u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        ASSERT((msr & 0xc0) == 0x80, ERROR_CONTROLLER_NEEDS_RESET); // Check that RQM = 1 and DIO = 0

        port_write_byte(DATA_FIFO, COMMAND_CONFIGURE);

        // Send parameters
        const u8 parameterBytes[3] = {0, t_parameter, 0}; // write precomp set to 0
        for (size_t i = 0; i < 3; i++) {
            msr = TRY(read_msr_until_rqm());
            ASSERT((msr & 0xc0) == 0x80, -1); // Check DIO = 0, TODO
            
            port_write_byte(DATA_FIFO, parameterBytes[i]);
        }

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> command_lock() {
        // Send command byte
        u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        ASSERT((msr & 0xc0) == 0x80, ERROR_CONTROLLER_NEEDS_RESET); // Check that RQM = 1 and DIO = 0
        
        port_write_byte(DATA_FIFO, COMMAND_LOCK | 0x80); // or with 0x80 to set lock bit

        // Read result byte
        const u8 result = port_read_byte(DATA_FIFO);
        msr = TRY(read_msr_until_rqm());
        ASSERT((msr & 0xe0) == 0x80, int(-1)); // Check that CMD BSY = 0 and DIO = 0

        // TODO: retry if fails here

        ASSERT(result == (1 << 4), -1); // Check that lock bit was set correctly, TODO

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> command_specify(u8 t_p1, u8 t_p2) {
        // Send command byte
        u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        ASSERT((msr & 0xc0) == 0x80, ERROR_CONTROLLER_NEEDS_RESET); // Check that RQM = 1 and DIO = 0
        
        port_write_byte(DATA_FIFO, COMMAND_SPECIFY);

        // Send parameters
        const u8 parameterBytes[2] = {t_p1, t_p2};
        for (size_t i = 0; i < 2; i++) {    
            msr = TRY(read_msr_until_rqm());
            ASSERT((msr & 0xc0) == 0x80, -1); // Check DIO = 0, TODO
            
            port_write_byte(DATA_FIFO, parameterBytes[i]);
        }

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> command_recalibrate(u8 t_drive) {
        ASSERT(t_drive < 4, -1); // TODO

        // Send command byte
        u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        ASSERT((msr & 0xc0) == 0x80, ERROR_CONTROLLER_NEEDS_RESET); // Check that RQM = 1 and DIO = 0

        port_write_byte(DATA_FIFO, COMMAND_RECALIBRATE);

        // Send parameter
        s_floppyState.irqType = IRQType::RECALIBRATE;

        msr = TRY(read_msr_until_rqm());
        ASSERT((msr & 0xc0) == 0x80, -1); // Check DIO = 0, TODO    
        port_write_byte(DATA_FIFO, t_drive);

        // Wait for IRQ
        for (size_t i = 0; i < RETRY_COUNT; i++) {
            if (s_floppyState.irqType == IRQType::RECALIBRATE) {
                return SZNN::ErrorOr<void>();
            }
            KERNEL_HALT();
            sleep(3000 / RETRY_COUNT);
        }

        return ERROR_TIMEOUT;
    }

    SZNN::ErrorOr<void> select_drive(u8 t_drive) {
        ASSERT(t_drive < 4, -1); // TODO

        port_write_byte(CONFIGURATION_CONTROL_REGISTER, 0); // set to 0 for 1.44MiB floppy
        TRY(command_specify((8 << 4) | 0, (5 << 1) | 0)); // SRT=8ms, HLT=10ms, HUT=0ms, NDMA=0 (using DMA)
        port_write_byte(DIGITAL_OUTPUT_REGISTER, 0x0C | t_drive);

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<u8> read_msr_until_rqm() {
        u8 msr = 0;
        for (size_t i = 0; i < RETRY_COUNT; i++) {
            msr = port_read_byte(MAIN_STATUS_REGISTER);

            if ((msr & 0x80) == 0x80) {
                return msr;
            }
        }
        return ERROR_TIMEOUT;
    }

    void floppy_handler(InterruptHandler::InterruptFrame* t_frame) {
        switch (s_floppyState.irqType) {
            case IRQType::RESET:
                break;
            case IRQType::RECALIBRATE:
                break;
            case IRQType::NONE:
                break;
            default:
                break;
        }
        s_floppyState.irqType = IRQType::NONE;

        PIC::send_end_of_interrupt(0);
    }

}
