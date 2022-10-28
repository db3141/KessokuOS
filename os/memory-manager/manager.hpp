#ifndef KERNEL_MEMORY_MANAGER_INCLUDED
#define KERNEL_MEMORY_MANAGER_INCLUDED

#include "common.hpp"
#include "error_code_groups.hpp"
#include "sznnlib/error_or.hpp"

namespace Kernel::MemoryManager {

    SZNN::ErrorOr<void> initialize();

    SZNN::ErrorOr<void*> malloc(size_t t_size);
    SZNN::ErrorOr<void> free(void* t_memory);

    void print_heap_information();
}

namespace Kernel {

    void* kmalloc(size_t t_size);
    void kfree(void* t_memory);

}

#endif
