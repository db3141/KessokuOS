#include "data/fc_vector.hpp"

#include "common.hpp"
#include "manager.hpp"
#include "drivers/vga.hpp"

namespace Kernel::MemoryManager {

    struct BlockHeader {
        BlockHeader* prev;
        BlockHeader* next;
        size_t size;
        bool used;
    };

    struct MemoryInfo {
        BlockHeader* baseNode;
        BlockHeader* tailNode;

        Data::FCVector<BlockHeader*, 2048> freeBlocks;
    };

    struct MemoryRange {
        u32 baseAddress;
        u32 regionLength;
    };

    struct MemoryRangeTable {
        MemoryRange entries[32];
        u32 entryCount;
    };


    extern "C" u8 _kernel_end[];

    constexpr size_t PAGE_SIZE = 4096; // TODO: change this
    u8* const HEAP_BASE_ADDRESS = _kernel_end - (int(_kernel_end) % PAGE_SIZE) + PAGE_SIZE;
    u64* const MEMORY_INFORMATION_TABLE = (u64*) 0x7000;

    static MemoryInfo s_memoryInfo;
    static MemoryRangeTable s_memoryRangeTable;
    

    void initialize_memory_range();
    size_t find_first_gte_free_block(size_t t_size);
    void add_free_block(BlockHeader* t_block);
    SZNN::ErrorOr<void> remove_free_block(BlockHeader* t_block);


    SZNN::ErrorOr<void> initialize() {
        initialize_memory_range();

        BlockHeader* block = reinterpret_cast<BlockHeader*>(HEAP_BASE_ADDRESS);
        *block = BlockHeader { nullptr, nullptr, 0, false };

        for (size_t i = 0; i < 32; i++) {
            const auto& entry = s_memoryRangeTable.entries[i];
            const u32 startAddress = entry.baseAddress;
            const u32 endAddress = startAddress + entry.regionLength;

            if (startAddress <= u32(HEAP_BASE_ADDRESS) && u32(HEAP_BASE_ADDRESS) < endAddress) {
                block->size = endAddress - startAddress - sizeof(BlockHeader);
                break;
            }
        }

        s_memoryInfo = MemoryInfo { block, block, {} };
        s_memoryInfo.freeBlocks.push_back(block);

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void*> malloc(size_t t_size) {
        ASSERT(s_memoryInfo.baseNode != nullptr, -1); // TODO: error code

        const size_t paddedSize = get_smallest_gte_multiple(t_size, sizeof(u32));
        const size_t freeBlockIndex = find_first_gte_free_block(paddedSize);

        ASSERT(freeBlockIndex < s_memoryInfo.freeBlocks.size(), -1); // TODO: error code

        BlockHeader* block = s_memoryInfo.freeBlocks[freeBlockIndex];
        s_memoryInfo.freeBlocks.remove(freeBlockIndex);

        block->used = true;

        if (block->size - paddedSize > sizeof(BlockHeader)) {
            BlockHeader* splitBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(block) + sizeof(BlockHeader) + paddedSize);
            *splitBlock = BlockHeader{ block, block->next, block->size - paddedSize - sizeof(BlockHeader), false };

            block->size = paddedSize;
            block->next = splitBlock;

            add_free_block(splitBlock);
        }

        return reinterpret_cast<u8*>(block) + sizeof(BlockHeader);
    }

    SZNN::ErrorOr<void> free(void* t_memory) {
        BlockHeader* node = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(t_memory) - sizeof(BlockHeader));

        ASSERT(node->used == true, -1); // TODO: error code

        node->used = false;

        if (node->next != nullptr && node->next->used == false) {
            TRY(remove_free_block(node->next));

            node->size += node->next->size + sizeof(BlockHeader);
            node->next = node->next->next;
            
            if (node->next != nullptr) {
                node->next->prev = node;
            }
        }

        if (node->prev != nullptr && node->prev->used == false) {
            TRY(remove_free_block(node->prev));

            node->prev->size += node->size + sizeof(BlockHeader);
            node->prev->next = node->next;
            
            if (node->next != nullptr) {
                node->next->prev = node->prev;
            }

            node = node->prev;
        }

        add_free_block(node);

        return SZNN::ErrorOr<void>();
    }

    void print_heap_information() {
        VGA::put_string("Blocks\n");
        VGA::put_string("------\n");
        for (const BlockHeader* node = s_memoryInfo.baseNode; node != nullptr; node = node->next) {
            VGA::put_string("Address: ");
            VGA::put_hex(int(node));
            VGA::put_string(", ");

            VGA::put_string("Size: ");
            VGA::put_unsigned_decimal(node->size);
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
            VGA::put_unsigned_decimal(s_memoryInfo.freeBlocks[i]->size);
            VGA::put_string(", ");

            VGA::put_string("Used: ");
            VGA::put_unsigned_decimal(s_memoryInfo.freeBlocks[i]->used);
            VGA::new_line();
        }
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
        if (s_memoryInfo.freeBlocks.size() == 0 || t_size <= s_memoryInfo.freeBlocks[0]->size) {
            return 0;
        }

        size_t start = 0;
        size_t end = s_memoryInfo.freeBlocks.size();

        size_t midpoint = (start + end) / 2;
        while (start != midpoint) {
            if (t_size > s_memoryInfo.freeBlocks[midpoint]->size) {
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
        const size_t index = find_first_gte_free_block(t_block->size);

        s_memoryInfo.freeBlocks.insert(index, t_block);
    }

    // TODO: improve the efficiency of this (e.g. second binary search over the addresses)
    SZNN::ErrorOr<void> remove_free_block(BlockHeader* t_block) {
        const size_t startIndex = find_first_gte_free_block(t_block->size);

        for (size_t i = startIndex; i < s_memoryInfo.freeBlocks.size() && t_block->size == s_memoryInfo.freeBlocks[i]->size; i++) {
            if (t_block == s_memoryInfo.freeBlocks[i]) {
                s_memoryInfo.freeBlocks.remove(i);
                return SZNN::ErrorOr<void>();
            }
        }

        ASSERT(false, -1); // TODO: error code
    }

}

namespace Kernel {

    void* kmalloc(size_t t_size) {
        SZNN::ErrorOr<void*> result = Kernel::MemoryManager::malloc(t_size);
        if (result.is_error()) {
            MemoryManager::print_heap_information();
            VGA::put_string("Failed to allocate memory of size: ");
            VGA::put_unsigned_decimal(t_size);
            VGA::new_line();
            KERNEL_STOP();
        }
        
        return result.get_value();
    }

    void kfree(void* t_memory) {
        SZNN::ErrorOr<void> result = Kernel::MemoryManager::free(t_memory);

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
