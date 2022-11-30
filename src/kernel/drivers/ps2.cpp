#include "common.hpp"
#include "interrupts/pic.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace Kernel::PS2 {

    constexpr u8 DATA_PORT = 0x60;
    constexpr u8 STATUS_REGISTER_PORT = 0x64;
    constexpr u8 COMMAND_REGISTER_PORT = 0x64;

    constexpr uint COMMAND_RETRY_COUNT = 3;

    enum Commands : u8 {
        DISABLE_FIRST_PS2  = 0xAD,
        ENABLE_FIRST_PS2   = 0xAE,

        DISABLE_SECOND_PS2 = 0xA7,
        ENABLE_SECOND_PS2  = 0xA8,

        READ_CONTROLLER_CONFIG_BYTE = 0x20,
        WRITE_CONTROLLER_CONFIG_BYTE = 0x60,

        PERFORM_SELF_TEST = 0xAA,

        TEST_FIRST_PS2_PORT = 0xAB,
        TEST_SECOND_PS2_PORT = 0xA9,
    };

    Data::ErrorOr<u8> get_response_immediate();
    Data::ErrorOr<void> send_to_device_immediate();

    Data::ErrorOr<u8> resend_until_success_or_timeout(u8 t_command);

    Data::ErrorOr<void> initialize() {
        // Disable any PS2 devices
        port_write_byte(COMMAND_REGISTER_PORT, DISABLE_FIRST_PS2);
        io_wait();
        port_write_byte(COMMAND_REGISTER_PORT, DISABLE_SECOND_PS2);
        io_wait();

        port_read_byte(DATA_PORT); // flush the output buffer
        io_wait();

        // Modify config byte
        port_write_byte(COMMAND_REGISTER_PORT, READ_CONTROLLER_CONFIG_BYTE);

        const u8 configByte = TRY(get_response());
        const bool dualChannel = false; // !!(configByte & 0b00100000);
        const u8 newConfigByte = configByte & 0b00110100; // clear bits 0,1 and 6 (disable interrupts and IRQs), and set bits 3 and 7 to 0 (required)

        port_write_byte(COMMAND_REGISTER_PORT, WRITE_CONTROLLER_CONFIG_BYTE);
        TRY(send_to_device(newConfigByte));
        io_wait();

        // Perform controller self test and restore config byte afterwards
        port_write_byte(COMMAND_REGISTER_PORT, PERFORM_SELF_TEST);
        if (TRY(get_response()) != 0x55) {
            VGA::put_string("PS2: Self test failed\n");
            return Error::DRIVER_DEVICE_CHECK_FAILED;
        }

        port_write_byte(COMMAND_REGISTER_PORT, WRITE_CONTROLLER_CONFIG_BYTE);
        TRY(send_to_device(newConfigByte));
        io_wait();

        // Perform interface tests
        port_write_byte(COMMAND_REGISTER_PORT, TEST_FIRST_PS2_PORT);
        if (TRY(get_response()) != 0x00) {
            VGA::put_string("PS2: first device interface test failed\n");
            return Error::DRIVER_DEVICE_CHECK_FAILED;
        }
        io_wait();

        if (dualChannel) {
            port_write_byte(COMMAND_REGISTER_PORT, TEST_SECOND_PS2_PORT);
            if (TRY(get_response()) != 0x00) {
                VGA::put_string("PS2: second device interface test failed\n");
                return Error::DRIVER_DEVICE_CHECK_FAILED;
            }
            io_wait();
        }

        // Enable devices
        port_write_byte(COMMAND_REGISTER_PORT, ENABLE_FIRST_PS2);
        io_wait();
        
        if (dualChannel) {
            port_write_byte(COMMAND_REGISTER_PORT, ENABLE_SECOND_PS2);
            io_wait();

            port_write_byte(COMMAND_REGISTER_PORT, WRITE_CONTROLLER_CONFIG_BYTE);
            TRY(send_to_device(newConfigByte | 0b00000011)); // enable interrupts for both PS2 ports
            io_wait();
        }
        else {
            port_write_byte(COMMAND_REGISTER_PORT, WRITE_CONTROLLER_CONFIG_BYTE);
            TRY(send_to_device(newConfigByte | 0b00000001)); // enable interrupts for the first PS2 port
            io_wait();
        }

        return Data::ErrorOr<void>();
    }

    Data::ErrorOr<void> send_to_device_immediate(u8 t_command) {
        const u8 status = port_read_byte(STATUS_REGISTER_PORT);
        if (status & 0x02) {
            return Error::CONTAINER_IS_FULL; // TODO: change this error
        }
        else {
            port_write_byte(DATA_PORT, t_command);
            return Data::ErrorOr<void>();
        }
    }

    Data::ErrorOr<void> send_to_device(u8 t_command) {
        auto result = send_to_device_immediate(t_command);
        for (size_t i = 0; i < COMMAND_RETRY_COUNT && result.is_error(); i++) {
            result = send_to_device_immediate(t_command);
        }
        return result;
    }

    Data::ErrorOr<u8> get_response_immediate() {
        const u8 status = port_read_byte(STATUS_REGISTER_PORT);

        // If there is no response then return error
        if (!(status & 0x01)) {
            return Error::DRIVER_DEVICE_NO_RESPONSE;
        }

        return port_read_byte(DATA_PORT);
    }

    Data::ErrorOr<u8> get_response() {
        auto result = get_response_immediate();
        for (size_t i = 0; i < 2 && result.is_error(); i++) {
            result = get_response_immediate();
        }
        return result;
    }

    Data::ErrorOr<DeviceType> get_first_port_device_type() {
        TRY(resend_until_success_or_timeout(0xF5)); // disable scanning
        TRY(resend_until_success_or_timeout(0xF2)); // send identify command

        const auto b1OrError = get_response(); // get response first byte
        if (b1OrError.is_error()) {
            if (b1OrError.get_error() == Error::DRIVER_DEVICE_NO_RESPONSE) {
                return DeviceType::AT_KEYBOARD;
            }
            else {
                return b1OrError.get_error();
            }
        }
        const u8 b1 = b1OrError.get_value();
        switch (b1) {
            case 0x00:
                return DeviceType::STANDARD_MOUSE;
            case 0x03:
                return DeviceType::MOUSE_WITH_SCROLLWHEEL;
            case 0x04:
                return DeviceType::FIVE_BUTTON_MOUSE;
            default:
                break;
        }

        if (b1 != 0xAB) {
            return Error::DRIVER_DEVICE_UNKNOWN;
        }

        const u8 b2 = TRY(get_response());
        switch (b2) {
            case 0x41:
                return DeviceType::MF2_KEYBOARD_TRANSLATION_ENABLED;
            case 0x83:
                return DeviceType::MF2_KEYBOARD;
            default:
                return Error::DRIVER_DEVICE_UNKNOWN;
        }
    }

    // TODO: implement this
    /*
    Data::ErrorOr<DeviceType> get_second_port_device_type() {

    }
    */

    Data::ErrorOr<u8> resend_until_success_or_timeout(u8 t_command) {
        for (size_t i = 0; i < COMMAND_RETRY_COUNT; i++) {
            TRY(send_to_device(t_command));

            const u8 response = TRY(get_response());
            if (response != 0xFE) {
                return response; // if not a resend response then return
            }
        }
        return Error::RETRY_LIMIT_REACHED;
    }

}
