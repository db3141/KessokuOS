#ifndef ERROR_CODE_GROUPS
#define ERROR_CODE_GROUPS

namespace Kernel::ErrorCodeGroup {

    enum ErrorCodeGroup : int {
        DATA_QUEUE = 0x00010000,

        DRIVERS_DMA          = 0x00020000
        DRIVERS_FLOPPY_DISK  = 0x00030000,
        DRIVERS_PS2          = 0x00040000,
        DRIVERS_PS2_KEYBOARD = 0x00050000,
    };

}

#endif
