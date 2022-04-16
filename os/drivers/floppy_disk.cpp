#include "drivers/dma.hpp"
#include "drivers/pit.hpp"
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
        COMMAND_READ_TRACK         = 2,         // generates IRQ6
        COMMAND_SPECIFY            = 3,         // * set drive parameters
        COMMAND_SENSE_DRIVE_STATUS = 4,
        COMMAND_WRITE_DATA         = 5 | 0x40,  // * write to the disk
        COMMAND_READ_DATA          = 6 | 0x40,  // * read from the disk
        COMMAND_RECALIBRATE        = 7,         // * seek to cylinder 0
        COMMAND_SENSE_INTERRUPT    = 8,         // * ack IRQ6, get status of last command
        COMMAND_WRITE_DELETED_DATA = 9,
        COMMAND_READ_ID            = 10,        // generates IRQ6
        COMMAND_READ_DELETED_DATA  = 12,
        COMMAND_FORMAT_TRACK       = 13 | 0x40, // *
        COMMAND_DUMPREG            = 14,
        COMMAND_SEEK               = 15,        // * seek both heads to cylinder X
        COMMAND_VERSION            = 16,        // * used during initialization, once
        COMMAND_SCAN_EQUAL         = 17,
        COMMAND_PERPENDICULAR_MODE = 18,        // * used during initialization, once, maybe
        COMMAND_CONFIGURE          = 19,        // * set controller parameters
        COMMAND_LOCK               = 20,        // * protect controller params from a reset
        COMMAND_VERIFY             = 22 | 0x40,
        COMMAND_SCAN_LOW_OR_EQUAL  = 25,
        COMMAND_SCAN_HIGH_OR_EQUAL = 29
    };

    static volatile struct {
        bool waitingForIRQ;
        u8 currentDrive;
        bool diskMotorOn[4];
    } s_floppyState;

    constexpr size_t MSR_READ_ATTEMPT_COUNT = 10; // TODO: maybe reduce this a bit
    constexpr size_t COMMAND_ATTEMPT_COUNT = 3;
    constexpr size_t TIMEOUT_TIME = 3 * PIT::TICKS_PER_SECOND;
    constexpr size_t DISK_SPINUP_WAIT_TIME = 300; // 300ms

    constexpr size_t DMA_BUFFER_SIZE = 0x2400;
    static u8 s_dmaBuffer[DMA_BUFFER_SIZE] __attribute__((aligned(0x10000))); // make sure buffer does not cross 64K boundary

    constexpr size_t PARAMETER_BUFFER_SIZE = 16;
    constexpr size_t RESULT_BUFFER_SIZE = 16;
    static u8 s_parameterBytes[PARAMETER_BUFFER_SIZE] = {0};
    static u8 s_resultBytes[RESULT_BUFFER_SIZE] = {0};

    SZNN::ErrorOr<void> send_command(Command t_command);

    SZNN::ErrorOr<void> select_drive(u8 t_drive, bool t_motorOn);

    SZNN::ErrorOr<u8> read_msr_until_rqm();
    SZNN::ErrorOr<void> wait_for_irq();

    // Base Case
    SZNN::ErrorOr<void> set_parameters(size_t t_index) {
        return SZNN::ErrorOr<void>();
    }

    template <typename T, typename ...Ts>
    SZNN::ErrorOr<void> set_parameters(size_t t_index, T t_parameter, Ts... t_rest) {
        ASSERT(t_index < PARAMETER_BUFFER_SIZE, -1); // TODO: error code
        s_parameterBytes[t_index] = t_parameter;

        return set_parameters(t_index + 1, t_rest...);
    }

    template <typename ...Ts>
    SZNN::ErrorOr<void> execute_command(Command t_command, Ts... t_parameters) {
        TRY(set_parameters(0, t_parameters...));

        for (size_t i = 0; i < COMMAND_ATTEMPT_COUNT; i++) {
            const auto result = send_command(t_command);
            if (!result.is_error()) {
                return SZNN::ErrorOr<void>();
            }
        }

        return ERROR_TIMEOUT;
    }

    constexpr size_t get_parameter_count(Command t_command) {
        switch (t_command) {
            case COMMAND_READ_TRACK:
                return 8;
            case COMMAND_SPECIFY:
                return 2;
            case COMMAND_SENSE_DRIVE_STATUS:
                return 1;
            case COMMAND_WRITE_DATA:
                return 8;
            case COMMAND_READ_DATA:
                return 8;
            case COMMAND_RECALIBRATE:
                return 1;
            case COMMAND_SENSE_INTERRUPT:
                return 0;
            case COMMAND_WRITE_DELETED_DATA:
                return 8;
            case COMMAND_READ_ID:
                return 1;
            case COMMAND_READ_DELETED_DATA:
                return 8;
            case COMMAND_FORMAT_TRACK:
                return 9; // TODO: check this is right
            case COMMAND_DUMPREG:
                return 0;
            case COMMAND_SEEK:
                return 2;
            case COMMAND_VERSION:
                return 0;
            case COMMAND_SCAN_EQUAL:
                return 8;
            case COMMAND_PERPENDICULAR_MODE:
                return 1;
            case COMMAND_CONFIGURE:
                return 3;
            case COMMAND_LOCK:
                return 0;
            case COMMAND_VERIFY:
                return 8;
            case COMMAND_SCAN_LOW_OR_EQUAL:
                return 8;
            case COMMAND_SCAN_HIGH_OR_EQUAL:
                return 8;
            default:
                return 0;
        }
    }

    constexpr size_t get_result_byte_count(Command t_command) {
        switch (t_command) {
            case COMMAND_READ_TRACK:
                return 7;
            case COMMAND_SPECIFY:
                return 0;
            case COMMAND_SENSE_DRIVE_STATUS:
                return 1;
            case COMMAND_WRITE_DATA:
                return 7;
            case COMMAND_READ_DATA:
                return 7;
            case COMMAND_RECALIBRATE:
                return 0;
            case COMMAND_SENSE_INTERRUPT:
                return 2;
            case COMMAND_WRITE_DELETED_DATA:
                return 7;
            case COMMAND_READ_ID:
                return 7;
            case COMMAND_READ_DELETED_DATA:
                return 7;
            case COMMAND_FORMAT_TRACK:
                return 7;
            case COMMAND_DUMPREG:
                return 10;
            case COMMAND_SEEK:
                return 0;
            case COMMAND_VERSION:
                return 1;
            case COMMAND_SCAN_EQUAL:
                return 7;
            case COMMAND_PERPENDICULAR_MODE:
                return 0;
            case COMMAND_CONFIGURE:
                return 0;
            case COMMAND_LOCK:
                return 1;
            case COMMAND_VERIFY:
                return 7;
            case COMMAND_SCAN_LOW_OR_EQUAL:
                return 7;
            case COMMAND_SCAN_HIGH_OR_EQUAL:
                return 7;
            default:
                return 0;
        }
    }

    constexpr bool command_has_interrupt(Command t_command) {
        return
            (t_command == COMMAND_VERIFY) ||
            (t_command == COMMAND_READ_DATA) ||
            (t_command == COMMAND_WRITE_DATA) ||
            (t_command == COMMAND_RECALIBRATE) ||
            (t_command == COMMAND_SEEK); // TODO: make sure all commands are included here!
    }

    SZNN::ErrorOr<void> initialize() {
        s_floppyState.waitingForIRQ = true;
        s_floppyState.currentDrive = -1;
        s_floppyState.diskMotorOn[0] = false;
        s_floppyState.diskMotorOn[1] = false;
        s_floppyState.diskMotorOn[2] = false;
        s_floppyState.diskMotorOn[3] = false;

        TRY(execute_command(COMMAND_VERSION));
        ASSERT(s_resultBytes[0] == 0x90, ERROR_UNSUPPORTED_VERSION);

        TRY(execute_command(COMMAND_CONFIGURE, 0x00, 0x57, 0x00)); // Implied seek on, FIFO on, drive polling mode off, threshold = 8 (7 + 1)
        TRY(execute_command(COMMAND_LOCK));
        TRY(reset());

        TRY(execute_command(COMMAND_VERSION)); // TODO: figure out why this helps

        TRY(select_drive(0, true)); // drive needs to be selected with motor on to recalibrate
        TRY(execute_command(COMMAND_RECALIBRATE, 0));
        TRY(execute_command(COMMAND_SENSE_INTERRUPT));

        TRY(DMA::initialize_channel(2, (void*)s_dmaBuffer, DMA_BUFFER_SIZE - 1)); // set-up DMA on channel 2 (floppy disk channel)

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> reset() {
        s_floppyState.waitingForIRQ = true; // Set state to be ready for a reset IRQ
        port_write_byte(DATARATE_SELECT_REGISTER, 0x80);

        TRY(wait_for_irq());

        TRY(select_drive(0, false)); // TODO: support more than one drive

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> read_data(u8 t_drive, u32 t_address, u32 t_count) {
        TRY(select_drive(t_drive, true));

        TRY(DMA::set_mode(2, 0b10, true, false, 0b01)); // prepare DMA channel for reading (TODO: try different transfer types)

        TRY(execute_command(COMMAND_READ_DATA, 0, 0, 0, 1, 2, 18, 0x1B, 0xFF));

        for (size_t i = 0; i < 8; i++) {
            VGA::put_hex(s_dmaBuffer[i]); VGA::new_line();
        }
        VGA::new_line();

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> send_command(Command t_command) {
        // Send command byte
        u8 msr = port_read_byte(MAIN_STATUS_REGISTER);
        ASSERT((msr & 0xc0) == 0x80, ERROR_CONTROLLER_NEEDS_RESET); // Check that RQM = 1 and DIO = 0

        port_write_byte(DATA_FIFO, t_command); // TODO: add check that command is valid

        // Send parameter bytes
        if (command_has_interrupt(t_command)) {
            s_floppyState.waitingForIRQ = true;
        }

        for (size_t i = 0; i < get_parameter_count(t_command); i++) {
            msr = TRY(read_msr_until_rqm());
            ASSERT((msr & 0xc0) == 0x80, -1); // TODO: error code

            port_write_byte(DATA_FIFO, s_parameterBytes[i]);
        }

        if (command_has_interrupt(t_command)) {
            TRY(wait_for_irq());
        }

        // Receive result bytes
        const size_t resultByteCount = get_result_byte_count(t_command);
        if (resultByteCount != 0) {
            for (size_t i = 0; i < resultByteCount - 1; i++) {
                s_resultBytes[i] = port_read_byte(DATA_FIFO);
                msr = TRY(read_msr_until_rqm());
                ASSERT((msr & 0x50) == 0x50, -1); // TODO: error code
            }

            s_resultBytes[resultByteCount - 1] = port_read_byte(DATA_FIFO);
            msr = TRY(read_msr_until_rqm());
            ASSERT((msr & 0x50) == 0x00, -1); // TODO: error code

            // TODO: retry command if fails here!
        }

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void> select_drive(u8 t_drive, bool t_motorOn) {
        ASSERT(t_drive < 4, -1); // TODO

        const u8 s1 = (8 << 4) | 0;
        const u8 s2 = (5 << 1) | 0;
        const u8 dor = ((!!t_motorOn) << (4 + t_drive)) | (0x0C | t_drive);

        if (s_floppyState.currentDrive != t_drive) {
            port_write_byte(CONFIGURATION_CONTROL_REGISTER, 0); // set to 0 for 1.44MiB floppy
            TRY(execute_command(COMMAND_SPECIFY, (8 << 4) | 0, (5 << 1) | 0)); // SRT=8ms, HLT=10ms, HUT=0ms, NDMA=0 (using DMA)
        }
        if (s_floppyState.currentDrive != t_drive || s_floppyState.diskMotorOn[t_drive] != t_motorOn) {
            port_write_byte(DIGITAL_OUTPUT_REGISTER, ((!!t_motorOn) << (4 + t_drive)) | (0x0C | t_drive));
        }

        s_floppyState.currentDrive = t_drive;
        s_floppyState.diskMotorOn[t_drive] = t_motorOn;

        // TRY(command_version()); // TODO: figure out why this makes it work
        sleep(DISK_SPINUP_WAIT_TIME); // wait for disk to spin up

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<u8> read_msr_until_rqm() {
        u8 msr = 0;
        for (size_t i = 0; i < MSR_READ_ATTEMPT_COUNT; i++) {
            msr = port_read_byte(MAIN_STATUS_REGISTER);

            if ((msr & 0x80) == 0x80) {
                return msr;
            }
        }
        return ERROR_TIMEOUT;
    }

    SZNN::ErrorOr<void> wait_for_irq() {
        const u32 t1 = PIT::get_ticks();
        while (PIT::get_ticks() - t1 < TIMEOUT_TIME) {
            if (!s_floppyState.waitingForIRQ) {
                return SZNN::ErrorOr<void>();
            }
            KERNEL_HALT();
        }

        return ERROR_TIMEOUT;
    }

    void floppy_handler(InterruptHandler::InterruptFrame* t_frame) {
        // VGA::put_string("IRQ6 Called\n");
        s_floppyState.waitingForIRQ = false;
        PIC::send_end_of_interrupt(0);
    }

}