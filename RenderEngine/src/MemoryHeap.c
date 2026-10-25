#include "MemoryHeap.h"
#include "DynamicArray.h"

// An eigthth of the block size will be devoted towards the queue tracking freed pointer locations and sizes
#define QUEUE_CAPACITY(size) ((size) >> 3)

#define ALIGN_POINTER(ptr, alignAmount) \
    (uintptr_t)(ptr) & (alignAmount - 1) == 0 ? (ptr) : (typeof(ptr))((uintptr_t)(ptr) - ((uintptr_t)(ptr) & (alignAmount - 1)) + alignAmount)

typedef struct QueueElement {
    uint8_t* ptr;
    size_t size;
} QueueElement;

#define ALLOCATION_METADATA_SIZE_INDEX 0
#define ALLOCATION_METADATA_HEAP_BLOCK_INDEX 1
#define ALLOCATION_METADATA_SIZE (2 * sizeof(uint64_t))
// uint64_t is done just to gaurentee a total size of 16 bytes for the metadata (which is done to match the base alignemnt)
typedef uint64_t* AllocationMetadata;

int MemoryHeap_create(MemoryHeap* heap, size_t size) {
    DYNAMIC_ARRAY_INIT((*heap));

    if (size < MEMORY_HEAP_BASE_ALIGN) {
        size = MEMORY_HEAP_BASE_ALIGN;
    }

    MemoryHeapBlock block;
    block.base = SDL_malloc(size);
    if (block.base == NULL) {
        SDL_Log("Failed to allocate memory for memory heap block: %s", SDL_GetError());
        return -1;
    }
    block.capacity = size;
    block.queue_capacity = QUEUE_CAPACITY(block.capacity);
    block.tracker = block.base + block.queue_capacity;
    block.tracker = ALIGN_POINTER(block.tracker, MEMORY_HEAP_BASE_ALIGN);
    block.queue_len = 0;

    DYNAMIC_ARRAY_APPEND((*heap), block);

    return 0;
}

void* MemoryHeap_alloc(MemoryHeap* heap, size_t size) {
    for (size_t i = 0; i < heap->len; i++) {
        QueueElement* elements = (QueueElement*)heap->arr[i].base;
        if (heap->arr[i].queue_len > 0) {
            for (size_t j = heap->arr[i].queue_len - 1; j >= 0; j--) {
                if (elements[j].size >= size) {
                    void* ptr = (void*)elements[i].ptr;
                    if (j == heap->arr[i].queue_len - 1) {
                        heap->arr[i].queue_len--;
                    } else {
                        SDL_memmove(&elements[j], &elements[j + 1], (heap->arr[i].queue_len - j - 1) * sizeof(QueueElement));
                    }
                    return ptr;
                }
            }
        }

        heap->arr[i].tracker = ALIGN_POINTER(heap->arr[i].tracker, MEMORY_HEAP_BASE_ALIGN);
        if (heap->arr[i].tracker - heap->arr[i].base + size <= heap->arr[i].capacity) {
            AllocationMetadata meta_data = (AllocationMetadata)heap->arr[i].tracker;
            meta_data[ALLOCATION_METADATA_SIZE_INDEX] = size;
            meta_data[ALLOCATION_METADATA_HEAP_BLOCK_INDEX] = i;
            heap->arr[i].tracker += ALLOCATION_METADATA_SIZE;
            void* ptr = (void*)(heap->arr[i].tracker);
            heap->arr[i].tracker += size;
            return ptr;
        }
    }

    MemoryHeapBlock newBlock = {0};
    newBlock.capacity = (3 * heap->arr[0].capacity) / 2;
    newBlock.base = SDL_malloc(newBlock.capacity);
    newBlock.tracker = newBlock.base;
    newBlock.queue_len = 0;
    newBlock.queue_capacity = QUEUE_CAPACITY(newBlock.capacity);

    newBlock.tracker = ALIGN_POINTER(newBlock.tracker, MEMORY_HEAP_BASE_ALIGN);
    AllocationMetadata meta_data = (AllocationMetadata)newBlock.tracker;
    meta_data[ALLOCATION_METADATA_SIZE_INDEX] = size;
    meta_data[ALLOCATION_METADATA_HEAP_BLOCK_INDEX] = heap->len;
    newBlock.tracker += ALLOCATION_METADATA_SIZE;
    void* ptr = (void*)(newBlock.tracker);
    newBlock.tracker += size;

    // Only append to dynamic array after finishing updating all values in the newBlock struct
    DYNAMIC_ARRAY_APPEND((*heap), newBlock);

    return ptr;
}

void* MemoryHeap_aligned_alloc(MemoryHeap* heap, size_t size, size_t align_to) {
    if (align_to < MEMORY_HEAP_BASE_ALIGN) {
        align_to = MEMORY_HEAP_BASE_ALIGN;
    }

    for (size_t i = 0; i < heap->len; i++) {
        QueueElement* elements = (QueueElement*)heap->arr[i].base;
        if (heap->arr[i].queue_len > 0) {
            for (size_t j = heap->arr[i].queue_len - 1; j >= 0; j--) {
                if (elements[j].size >= size) {
                    void* ptr = (void*)elements[i].ptr;
                    if (j == heap->arr[i].queue_len - 1) {
                        heap->arr[i].queue_len--;
                    } else {
                        SDL_memmove(&elements[j], &elements[j + 1], (heap->arr[i].queue_len - j - 1) * sizeof(QueueElement));
                    }
                    return ptr;
                }
            }
        }

        heap->arr[i].tracker = ALIGN_POINTER(heap->arr[i].tracker, align_to);
        if (heap->arr[i].tracker - heap->arr[i].base + size <= heap->arr[i].capacity) {
            AllocationMetadata meta_data = (AllocationMetadata)heap->arr[i].tracker;
            meta_data[ALLOCATION_METADATA_SIZE_INDEX] = size;
            meta_data[ALLOCATION_METADATA_HEAP_BLOCK_INDEX] = i;
            heap->arr[i].tracker += ALLOCATION_METADATA_SIZE;
            void* ptr = (void*)(heap->arr[i].tracker);
            heap->arr[i].tracker += size;
            return ptr;
        }
    }

    MemoryHeapBlock newBlock = {0};
    newBlock.capacity = (3 * heap->arr[0].capacity) / 2;
    newBlock.base = SDL_malloc(newBlock.capacity);
    newBlock.tracker = newBlock.base;
    newBlock.queue_len = 0;
    newBlock.queue_capacity = QUEUE_CAPACITY(newBlock.capacity);

    newBlock.tracker = ALIGN_POINTER(newBlock.tracker, align_to);
    AllocationMetadata meta_data = (AllocationMetadata)newBlock.tracker;
    meta_data[ALLOCATION_METADATA_SIZE_INDEX] = size;
    meta_data[ALLOCATION_METADATA_HEAP_BLOCK_INDEX] = heap->len;
    newBlock.tracker += ALLOCATION_METADATA_SIZE;
    void* ptr = newBlock.tracker;
    newBlock.tracker += size;

    // Only append to dynamic array after finishing updating all values in the newBlock struct
    DYNAMIC_ARRAY_APPEND((*heap), newBlock);

    return ptr;
}

void MemoryHeap_free(MemoryHeap* heap, void* ptr) {
    AllocationMetadata meta_data = (AllocationMetadata)((uint8_t*)ptr - ALLOCATION_METADATA_SIZE);
    size_t heap_block_index = meta_data[ALLOCATION_METADATA_HEAP_BLOCK_INDEX];

    if ((heap->arr[heap_block_index].queue_len + 1) * sizeof(QueueElement) < heap->arr[heap_block_index].capacity) {
        ((QueueElement*)heap->arr[heap_block_index].base)[heap->arr[heap_block_index].queue_len] = (QueueElement){
            .ptr = (uint8_t*)ptr,
            .size = meta_data[ALLOCATION_METADATA_SIZE_INDEX],
        };
        heap->arr[heap_block_index].queue_len++;
    }
}

int MemoryHeap_destroy(MemoryHeap* heap) {
    for (size_t i = 0; i < heap->len; i++) {
        SDL_free(heap->arr[i].base);
    }
    DYNAMIC_ARRAY_FREE((*heap));
    return 0;
}