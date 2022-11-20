#ifndef KERNEL_ERROR
#define KERNEL_ERROR

#define ERROR_CODE_NAMES(DO) \
    DO(INVALID_ARGUMENT)\
    DO(INDEX_OUT_OF_RANGE)\
    DO(RETRY_LIMIT_REACHED)\
    DO(TIMED_OUT)\
    DO(NOT_IMPLEMENTED)\
    DO(UNINITIALIZED)\
    \
    DO(DRIVER_DEVICE_NEEDS_RESET)\
    DO(DRIVER_DEVICE_NO_RESPONSE)\
    DO(DRIVER_DEVICE_CHECK_FAILED)\
    DO(DRIVER_COMMAND_FAILED)\
    DO(DRIVER_DEVICE_UNKNOWN)\
    DO(DRIVER_INVALID_DEVICE)\
    \
    DO(MEMORY_MANAGER_NO_FREE_BLOCKS)\
    DO(MEMORY_MANAGER_FAILED_TO_FIND_MEMORY_REGION)\
    \
    DO(CONTAINER_IS_FULL)\
    DO(CONTAINER_IS_EMPTY)

#define MAKE_ENUM(VAR) VAR,
#define MAKE_STRING(VAR) #VAR,

namespace Kernel {

    enum class Error : int {
        ERROR_CODE_NAMES(MAKE_ENUM)
    };

    constexpr const char* get_error_string(Error t_error) {
        constexpr const char* ERROR_CODE_STRINGS[] = {
            ERROR_CODE_NAMES(MAKE_STRING)
        };

        return ERROR_CODE_STRINGS[static_cast<int>(t_error)];
    }

}

#undef MAKE_ENUM
#undef MAKE_STRING
#undef ERROR_CODE_NAMES

#endif
