#ifndef PS2_INCLUDED
#define PS2_INCLUDED

#include "data/error_or.hpp"

#include "interrupts/interrupt_handler.hpp"

namespace Kernel::PS2 {

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
            default:
                return nullptr;
        }
    }

    Data::ErrorOr<void> initialize();

    //void reset_devices();
    Data::ErrorOr<void> send_to_device(u8 t_command);
    Data::ErrorOr<u8> get_response();

    Data::ErrorOr<u8> resend_until_success_or_timeout(u8 t_command);
    
    Data::ErrorOr<DeviceType> get_first_port_device_type();
    // Data::ErrorOr<DeviceType> get_second_port_device_type();

}

#endif
