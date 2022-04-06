#include "sznnlib/error_or.hpp"
#include "pic.hpp"
#include "floppy_disk.hpp"

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

    static volatile struct {
        bool initialized;
        bool resetWait;
        bool specifyRequired;
        u8 currentDrive;
    } s_floppyState;

    SZNN::ErrorOr<void> send_parameter(u8 t_parameter);
    SZNN::ErrorOr<void> select_drive(u8 t_drive);

    SZNN::ErrorOr<u8> command_version();
    SZNN::ErrorOr<void> command_configure();
    SZNN::ErrorOr<void> command_lock();

    SZNN::ErrorOr<void> initialize() {
        s_floppyState.initialized = false;
        s_floppyState.specifyRequired = true;

        const u8 version = TRY(command_version());
        ASSERT(version == 0x90, ERROR_UNSUPPORTED_VERSIONS); // TODO: error codes

        TRY(command_configure());
        TRY(command_lock(true));

        TRY(reset());

        // TODO: recalibrate
        
        s_floppyState.initialized = true;
    }

    SZNN::ErrorOr<void> reset() {
        s_floppyState.resetWait = true;

        const u8 dsr = 0;
        port_write_byte(DATARATE_SELECT_REGISTER, dsr | 0x80);
        while (s_floppyState.resetWait) {
            KERNEL_HALT();
        }

        s_floppyState.specifyRequired = true;
        select_drive(s_floppyState.currentDrive);
    }

    INTERRUPT_HANDLER void floppy_handler(InterruptHandler::InterruptFrame* t_frame) {
        s_floppyState.resetWait = false;
        PIC::send_end_of_interrupt(0x26);
    }

    SZNN::ErrorOr<void> send_parameter(u8 t_parameter) {
        for (size_t i = 0; i < 20 ; i++) {
            const u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
            io_wait();

            if (msr & 0x80) {
                if (!(msr & 0x40)) {
                    port_write_byte(DATA_FIFO, t_parameter);
                    io_wait();
                    return SZNN::ErrorOr<void>();
                }
                else {
                    return ERROR_CONTROLLER_NOT_EXPECTING_WRITE;
                }
            }
            
        }
        return ERROR_SEND_TIMEOUT;
    }

    SZNN::ErrorOr<void> send_parameters() {
        ;
    }

    template <typename Parameter, typename... Parameters>
    SZNN::ErrorOr<void> send_parameters(Parameter t_param, Parameters... t_params) {
        TRY(send_parameter(t_param));
        send_parameters(t_params...);
    }

    template <typename... Parameters>
    SZNN::ErrorOr<void> send_command(u8 t_command, Parameters... t_params) {
        const u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        io_wait();
        if ((msr & 0xC0) != 0x80) {
            // TODO: reset and try again
            return ERROR_CONTROLLER_NOT_EXPECTING_WRITE;
        }

        port_write_byte(DATA_FIFO, t_command);
        io_wait();

        send_parameters(t_params...);        
    }


    SZNN::ErrorOr<void> select_drive(u8 t_drive) {
        ASSERT(t_drive < 4, ERROR_INVALID_DRIVE);

        port_write_byte(CONFIGURATION_CONTROL_REGISTER, 0); // 1.44 MB floppy
        io_wait();

        if (s_floppyState.specifyRequired || t_drive != s_floppyState.currentDrive) {
            s_floppyState.currentDrive = t_drive;
            // TODO: send specify command
        }

        port_write_byte(DIGITAL_OUTPUT_REGISTER, 0x0C | t_drive);
        io_wait();
    }

    SZNN::ErrorOr<u8> command_version() {
        TRY(send_command(COMMAND_VERSION));
        return port_read_byte(DATA_FIFO);
    }

    SZNN::ErrorOr<void> command_configure() {
        const u8 configureByte = (1 << 6) | (0 << 5) | (1 << 4) | (8 - 1); // TODO: replace magic numbers with parameters
        TRY(send_command(COMMAND_CONFIGURE, 0, configureByte, 0));
    }

    SZNN::ErrorOr<void> command_lock(bool t_lock) {
        const u8 commandByte = COMMAND_LOCK | (!!t_lock << 7);
        TRY(send_command(commandByte));
        ASSERT(port_read_byte(DATA_FIFO) == (!!t_lock << 4), ERROR_LOCK_FAILED);
    }

}
