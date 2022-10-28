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


    SZNN::ErrorOr<void> initialize() {
        initialize_memory_range();
        s_memoryInfo = MemoryInfo { nullptr, nullptr, {} };

        VGA::put_string("Kernel End: ");
        VGA::put_hex(int(_kernel_end));
        VGA::new_line();

        VGA::put_string("Heap Start: ");
        VGA::put_hex(int(HEAP_BASE_ADDRESS));
        VGA::new_line();

        return SZNN::ErrorOr<void>();
    }

    SZNN::ErrorOr<void*> malloc(size_t t_size) {
        const size_t paddedSize = get_smallest_gte_multiple(t_size, sizeof(u32));
        BlockHeader* block = nullptr;

        if (s_memoryInfo.baseNode == nullptr) {
            s_memoryInfo.baseNode = reinterpret_cast<BlockHeader*>(HEAP_BASE_ADDRESS);
            s_memoryInfo.tailNode = s_memoryInfo.baseNode;

            *s_memoryInfo.baseNode = BlockHeader{ nullptr, nullptr, paddedSize, true };

            block = s_memoryInfo.tailNode;
        }
        else {
            const size_t freeBlockIndex = find_first_gte_free_block(paddedSize);

            if (freeBlockIndex < s_memoryInfo.freeBlocks.size()) {
                block = s_memoryInfo.freeBlocks[freeBlockIndex];
                block->used = true;

                s_memoryInfo.freeBlocks.remove(freeBlockIndex);

                if (block->size - paddedSize > sizeof(BlockHeader)) {
                    BlockHeader* splitBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(block) + sizeof(BlockHeader) + paddedSize);
                    *splitBlock = BlockHeader{ block, block->next, block->size - paddedSize - sizeof(BlockHeader), false };

                    block->size = paddedSize;
                    block->next = splitBlock;

                    add_free_block(splitBlock);
                }

            }
            else {
                ASSERT(s_memoryInfo.tailNode != nullptr, -1); // TODO: error code

                block = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(s_memoryInfo.tailNode) + sizeof(BlockHeader) + s_memoryInfo.tailNode->size);
                *block = BlockHeader{ s_memoryInfo.tailNode, nullptr, paddedSize, true };

                s_memoryInfo.tailNode->next = block;
                s_memoryInfo.tailNode = block;
            }
        }

        return reinterpret_cast<u8*>(block) + sizeof(BlockHeader);
    }

    SZNN::ErrorOr<void> free(void* t_memory) {
        BlockHeader* node = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(t_memory) - sizeof(BlockHeader));

        ASSERT(node->used == true, -1); // TODO: error code

        node->used = false;

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

}

namespace Kernel {

    void* kmalloc(size_t t_size) {
        SZNN::ErrorOr<void*> result = Kernel::MemoryManager::malloc(t_size);
        if (result.is_error()) {
            VGA::put_string("Failed to allocate memory\n");
            KERNEL_STOP();
        }
        
        return result.get_value();
    }

    void kfree(void* t_memory) {
        SZNN::ErrorOr<void> result = Kernel::MemoryManager::free(t_memory);

        if (result.is_error()) {
            VGA::put_string("Failed to free memory\n");
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
