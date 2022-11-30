#include "data/fc_vector.hpp"

#include "common.hpp"
#include "manager.hpp"
#include "drivers/vga.hpp"

namespace Kernel::MemoryManager {

    struct BlockHeader {
        BlockHeader* prev;
        BlockHeader* next;
        bool used;
    };

    struct MemoryInfo {
        BlockHeader* baseNode;
        BlockHeader* tailNode;
        u8* endAddress;

        Data::FCVector<BlockHeader*, 2048> freeBlocks;
    };

    struct MemoryRange {
        uintptr_t baseAddress;
        size_t regionLength;
    };

    // TODO: Replace this with an array class
    struct MemoryRangeTable {
        MemoryRange entries[32];
        size_t entryCount;
    };


    extern "C" u8 _kernel_end[];

    constexpr size_t PAGE_SIZE = 4096; // TODO: change this
    u8* const HEAP_BASE_ADDRESS = _kernel_end - (reinterpret_cast<uintptr_t>(_kernel_end) % PAGE_SIZE) + PAGE_SIZE;
    u64* const MEMORY_INFORMATION_TABLE = reinterpret_cast<u64*>(0x7000);

    static MemoryInfo s_memoryInfo;
    static MemoryRangeTable s_memoryRangeTable;
    

    void initialize_memory_range();

    size_t find_first_gte_free_block(size_t t_size);

    void add_free_block(BlockHeader* t_block);
    Data::ErrorOr<void> remove_free_block(BlockHeader* t_block);

    size_t get_block_size(const BlockHeader* t_block);


    Data::ErrorOr<void> initialize() {
        initialize_memory_range();

        BlockHeader* block = reinterpret_cast<BlockHeader*>(HEAP_BASE_ADDRESS);
        *block = BlockHeader { nullptr, nullptr, false };

        u8* memEndAddress = nullptr;
        for (size_t i = 0; i < 32; i++) {
            const auto& entry = s_memoryRangeTable.entries[i];
            const uintptr_t startAddress = entry.baseAddress;
            const uintptr_t endAddress = startAddress + entry.regionLength;

            // TODO: change this to use more than one memory region
            if (startAddress <= reinterpret_cast<uintptr_t>(HEAP_BASE_ADDRESS) && reinterpret_cast<uintptr_t>(HEAP_BASE_ADDRESS) < endAddress) {
                memEndAddress = reinterpret_cast<u8*>(endAddress);
                break;
            }
        }

        ASSERT(memEndAddress != nullptr, Error::MEMORY_MANAGER_FAILED_TO_FIND_MEMORY_REGION);

        s_memoryInfo = MemoryInfo { block, block, memEndAddress, {} };
        s_memoryInfo.freeBlocks.push_back(block);

        return Data::ErrorOr<void>();
    }

    Data::ErrorOr<void*> malloc(size_t t_size) {
        ASSERT(t_size != 0, Error::INVALID_ARGUMENT);
        ASSERT(s_memoryInfo.baseNode != nullptr, Error::UNINITIALIZED);

        const size_t paddedSize = get_smallest_gte_multiple(t_size, sizeof(u32));
        const size_t freeBlockIndex = find_first_gte_free_block(paddedSize);

        ASSERT(freeBlockIndex < s_memoryInfo.freeBlocks.size(), Error::MEMORY_MANAGER_NO_FREE_BLOCKS);

        BlockHeader* block = s_memoryInfo.freeBlocks[freeBlockIndex];
        s_memoryInfo.freeBlocks.remove(freeBlockIndex);

        block->used = true;

        if (get_block_size(block) - paddedSize > sizeof(BlockHeader)) {
            BlockHeader* splitBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(block) + sizeof(BlockHeader) + paddedSize);
            *splitBlock = BlockHeader{ block, block->next, false };

            block->next = splitBlock;

            add_free_block(splitBlock);
        }

        return reinterpret_cast<u8*>(block) + sizeof(BlockHeader);
    }

    Data::ErrorOr<void> free(void* t_memory) {
        if (t_memory == nullptr) {
            return Data::ErrorOr<void>();
        }

        BlockHeader* node = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(t_memory) - sizeof(BlockHeader));

        ASSERT(node->used == true, Error::INVALID_ARGUMENT);

        node->used = false;

        if (node->next != nullptr && node->next->used == false) {
            TRY(remove_free_block(node->next));

            node->next = node->next->next;
            
            if (node->next != nullptr) {
                node->next->prev = node;
            }
        }

        if (node->prev != nullptr && node->prev->used == false) {
            TRY(remove_free_block(node->prev));

            node->prev->next = node->next;
            
            if (node->next != nullptr) {
                node->next->prev = node->prev;
            }

            node = node->prev;
        }

        add_free_block(node);

        return Data::ErrorOr<void>();
    }

    void print_heap_information() {
        VGA::put_string("Blocks\n");
        VGA::put_string("------\n");
        for (const BlockHeader* node = s_memoryInfo.baseNode; node != nullptr; node = node->next) {
            VGA::put_string("Address: ");
            VGA::put_hex(reinterpret_cast<uintptr_t>(node));
            VGA::put_string(", ");

            VGA::put_string("Size: ");
            VGA::put_unsigned_decimal(get_block_size(node));
            VGA::put_string(", ");

            VGA::put_string("Used: ");
            VGA::put_unsigned_decimal(node->used);
            VGA::new_line();
        }

        VGA::put_string("\nFree Blocks\n");
        VGA::put_string("------------\n");
        for (size_t i = 0; i < s_memoryInfo.freeBlocks.size(); i++) {
            VGA::put_string("Address: ");
            VGA::put_hex(int(s_memoryInfo.freeBlocks[i]));
            VGA::put_string(", ");

            VGA::put_string("Size: ");
            VGA::put_unsigned_decimal(get_block_size(s_memoryInfo.freeBlocks[i]));
            VGA::put_string(", ");

            VGA::put_string("Used: ");
            VGA::put_unsigned_decimal(s_memoryInfo.freeBlocks[i]->used);
            VGA::new_line();
        }

        VGA::new_line();
    }

    void print_memory_range_information() {
        VGA::put_string("Memory Ranges\n---------------\n");

        for (size_t i = 0; i < s_memoryRangeTable.entryCount; i++) {
            const auto& entry = s_memoryRangeTable.entries[i];

            VGA::put_hex(entry.baseAddress);
            VGA::put_string(" - ");
            VGA::put_hex(entry.baseAddress + entry.regionLength);
            VGA::new_line();
        }

        VGA::new_line();
    }

    void initialize_memory_range() {
        s_memoryRangeTable.entryCount = 0; 
        for (u64* p = MEMORY_INFORMATION_TABLE; ; p += 3) {
            // If we have reached the NULL entry or there is no more space in the table then stop
            if ((p[0] == 0 && p[1] == 0 && p[2] == 0) || s_memoryRangeTable.entryCount >= 32) {
                break;
            }
            
            const MemoryRange range = { static_cast<u32>(p[0] & 0xFFFFFFFF) , static_cast<u32>(p[1] & 0xFFFFFFFF) };
            const u32 regionType = p[2] & 0xFFFFFFFF;
            if (regionType == 1) {
                s_memoryRangeTable.entries[s_memoryRangeTable.entryCount] = range;
                s_memoryRangeTable.entryCount++;
            }
        }
    }

    size_t find_first_gte_free_block(size_t t_size) {
        if (s_memoryInfo.freeBlocks.size() == 0 || t_size <= get_block_size(s_memoryInfo.freeBlocks[0])) {
            return 0;
        }

        size_t start = 0;
        size_t end = s_memoryInfo.freeBlocks.size();

        size_t midpoint = (start + end) / 2;
        while (start != midpoint) {
            if (t_size > get_block_size(s_memoryInfo.freeBlocks[midpoint])) {
                start = midpoint;
            }
            else {
                end = midpoint;
            }

            midpoint = (start + end) / 2;
        };

        return end;
    }

    void add_free_block(BlockHeader* t_block) {
        const size_t index = find_first_gte_free_block(get_block_size(t_block));

        s_memoryInfo.freeBlocks.insert(index, t_block);
    }

    // TODO: improve the efficiency of this (e.g. second binary search over the addresses)
    Data::ErrorOr<void> remove_free_block(BlockHeader* t_block) {
        const size_t blockSize = get_block_size(t_block);
        const size_t startIndex = find_first_gte_free_block(blockSize);

        for (size_t i = startIndex; i < s_memoryInfo.freeBlocks.size() && blockSize == get_block_size(s_memoryInfo.freeBlocks[i]); i++) {
            if (t_block == s_memoryInfo.freeBlocks[i]) {
                s_memoryInfo.freeBlocks.remove(i);
                return Data::ErrorOr<void>();
            }
        }

        return Data::ErrorOr<void>(Error::INDEX_OUT_OF_RANGE);
    }

    size_t get_block_size(const BlockHeader* t_block) {
        if (t_block == nullptr) {
            return 0;
        }

        const u8* endAddress = (t_block->next == nullptr) ? (s_memoryInfo.endAddress) : (reinterpret_cast<u8*>(t_block->next));

        return (endAddress - reinterpret_cast<const u8*>(t_block)) - sizeof(BlockHeader);
    }

}

namespace Kernel {

    void* kmalloc(size_t t_size) {
        Data::ErrorOr<void*> result = Kernel::MemoryManager::malloc(t_size);
        if (result.is_error()) {
            MemoryManager::print_heap_information();
            VGA::put_string("Failed to allocate memory of size: ");
            VGA::put_unsigned_decimal(t_size);
            VGA::put_string(" bytes");
            VGA::new_line();
            KERNEL_STOP();
        }
        
        return result.get_value();
    }

    void kfree(void* t_memory) {
        Data::ErrorOr<void> result = Kernel::MemoryManager::free(t_memory);

        if (result.is_error()) {
            MemoryManager::print_heap_information();
            VGA::put_string("Failed to free address: ");
            VGA::put_hex(int(t_memory));
            VGA::new_line();
            KERNEL_STOP();
        }
    }

}

void* operator new(size_t t_size) {
    return Kernel::kmalloc(t_size);
}

void* operator new[](size_t t_size) {
    return Kernel::kmalloc(t_size);
}

void operator delete(void* t_memory) {
    Kernel::kfree(t_memory);
}

void operator delete[](void* t_memory) {
    Kernel::kfree(t_memory);
}

void operator delete(void* t_memory, size_t t_size) {
    (void)t_size; // stop compiler complaining about unused parameter
    Kernel::kfree(t_memory);
}

void operator delete[](void* t_memory, size_t t_size) {
    (void)t_size; // stop compiler complaining about unused parameter
    Kernel::kfree(t_memory);
}
