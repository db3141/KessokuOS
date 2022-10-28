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
        u8* basePtr;
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

    u64* const MEMORY_INFORMATION_TABLE = (u64*) 0x7000;
    constexpr size_t PAGE_SIZE = 4096; // TODO: change this

    static MemoryInfo s_memoryInfo;
    static MemoryRangeTable s_memoryRangeTable;
    

    void initialize_memory_range();


    SZNN::ErrorOr<void> initialize() {
        u8* const HEAP_BASE_ADDRESS = _kernel_end - (int(_kernel_end) % PAGE_SIZE) + PAGE_SIZE;

        initialize_memory_range();
        s_memoryInfo = MemoryInfo { HEAP_BASE_ADDRESS, nullptr, nullptr, {} };

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

        if (s_memoryInfo.baseNode == nullptr) {
            s_memoryInfo.baseNode = reinterpret_cast<BlockHeader*>(s_memoryInfo.basePtr);
            s_memoryInfo.tailNode = s_memoryInfo.baseNode;

            *s_memoryInfo.baseNode = BlockHeader{ nullptr, nullptr, paddedSize, true };
        }
        else {
            ASSERT(s_memoryInfo.tailNode != nullptr, -1); // TODO: error code

            BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(s_memoryInfo.tailNode) + sizeof(BlockHeader) + s_memoryInfo.tailNode->size);

            *newBlock = BlockHeader{ s_memoryInfo.tailNode, nullptr, paddedSize, true };
            s_memoryInfo.tailNode->next = newBlock;

            s_memoryInfo.tailNode = newBlock;
        }

        return reinterpret_cast<u8*>(s_memoryInfo.tailNode) + sizeof(BlockHeader);
    }

    SZNN::ErrorOr<void> free(void* t_memory) {
        BlockHeader* node = reinterpret_cast<BlockHeader*>(reinterpret_cast<u8*>(t_memory) - sizeof(BlockHeader));

        ASSERT(node->used == true, -1); // TODO: error code

        node->used = false;

        return SZNN::ErrorOr<void>();
    }

    void print_heap_information() {
        VGA::put_string("-------------------\n");
        VGA::put_string("Heap Information\n");
        for (const BlockHeader* node = s_memoryInfo.baseNode; node != nullptr; node = node->next) {
            VGA::put_string("-------------------\n");

            VGA::put_string("Address: ");
            VGA::put_hex(int(node));
            VGA::new_line();

            VGA::put_string("Size: ");
            VGA::put_unsigned_decimal(node->size);
            VGA::new_line();

            VGA::put_string("Used: ");
            VGA::put_unsigned_decimal(node->used);
            VGA::new_line();
        }
        VGA::put_string("-------------------\n");
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

