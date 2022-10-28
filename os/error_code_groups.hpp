#ifndef ERROR_CODE_GROUPS
#define ERROR_CODE_GROUPS

namespace Kernel::ErrorCodeGroup {

    enum class Group : int {
        GENERIC,

        DATA_QUEUE,
        DATA_FC_VECTOR,

        DRIVERS_DMA,
        DRIVERS_FLOPPY_DISK,
        DRIVERS_PS2,
        DRIVERS_PS2_KEYBOARD,
    };

    constexpr int get_id(Group t_group) {
        return int(t_group) << ((8 * sizeof(int)) / 2);
    }

}

#endif
