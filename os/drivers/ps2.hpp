#ifndef PS2_INCLUDED
#define PS2_INCLUDED

#include "error_code_groups.hpp"
#include "sznnlib/error_or.hpp"

#include "interrupts/interrupt_handler.hpp"

namespace Kernel::PS2 {

    enum Error : int {
        ERROR_INPUT_BUFFER_FULL = ErrorCodeGroup::get_id(ErrorCodeGroup::Group::DRIVERS_PS2),
        ERROR_OUTPUT_BUFFER_EMPTY,
        ERROR_NO_RESPONSE,
        ERROR_RESEND_LIMIT_REACHED,

        ERROR_UNKNOWN_DEVICE_TYPE,

        ERROR_DEVICE_SELF_TEST_FAILED,
        ERROR_DEVICE1_INTERFACE_TEST_FAILED,
        ERROR_DEVICE2_INTERFACE_TEST_FAILED,
    };

    enum class DeviceType {
        AT_KEYBOARD,
        STANDARD_MOUSE,
        MOUSE_WITH_SCROLLWHEEL,
        FIVE_BUTTON_MOUSE,
        MF2_KEYBOARD_TRANSLATION_ENABLED,
        MF2_KEYBOARD
    };

    constexpr const char* device_type_string(DeviceType t_deviceType) {
        switch (t_deviceType) {
            case DeviceType::AT_KEYBOARD:
                return "AT Keyboard";
            case DeviceType::STANDARD_MOUSE:
                return "Standard Mouse";
            case DeviceType::MOUSE_WITH_SCROLLWHEEL:
                return "Mouse with Scrollwheel";
            case DeviceType::FIVE_BUTTON_MOUSE:
                return "5 Button Mouse";
            case DeviceType::MF2_KEYBOARD_TRANSLATION_ENABLED:
                return "MF2 Keyboard with Translation Enabled";
            case DeviceType::MF2_KEYBOARD:
                return "MF2 Keyboard";
        }
    }

    SZNN::ErrorOr<void> initialize();

    //void reset_devices();
    SZNN::ErrorOr<void> send_to_device(u8 t_command);
    SZNN::ErrorOr<u8> get_response();

    SZNN::ErrorOr<u8> resend_until_success_or_timeout(u8 t_command);
    
    SZNN::ErrorOr<DeviceType> get_first_port_device_type();
    // SZNN::ErrorOr<DeviceType> get_second_port_device_type();

}

#endif
