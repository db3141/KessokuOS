#ifndef KERNEL_MEMORY_MANAGER_INCLUDED
#define KERNEL_MEMORY_MANAGER_INCLUDED

#include "common.hpp"
#include "data/error_or.hpp"

namespace Kernel::MemoryManager {

    Data::ErrorOr<void> initialize();

    Data::ErrorOr<void*> malloc(size_t t_size);
    Data::ErrorOr<void> free(void* t_memory);

    void print_memory_range_information();
    void print_heap_information();
}

namespace Kernel {

    void* kmalloc(size_t t_size);
    void kfree(void* t_memory);

}

void* operator new(size_t t_size);
void* operator new[](size_t t_size);

void operator delete(void* t_memory);
void operator delete[](void* t_memory);
void operator delete(void* t_memory, size_t t_size);
void operator delete[](void* t_memory, size_t t_size);


#endif
