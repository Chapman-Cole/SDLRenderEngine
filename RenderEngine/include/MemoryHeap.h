#ifndef MEMORYHEAP_H
#define MEMORYHEAP_H

#include "SDL3/SDL.h"

#define MEMORY_HEAP_BASE_ALIGN 16

typedef struct MemoryHeapBlock {
    uint8_t* base;
    uint8_t* tracker;
    size_t capacity;
    size_t queue_len;
    size_t queue_capacity;
} MemoryHeapBlock;

typedef struct MemoryHeap {
    MemoryHeapBlock* arr;
    size_t len;
    size_t capacity;
} MemoryHeap;

int MemoryHeap_create(MemoryHeap* heap, size_t size);

void* MemoryHeap_alloc(MemoryHeap* heap, size_t size);

void* MemoryHeap_aligned_alloc(MemoryHeap* heap, size_t size, size_t align_to);

void MemoryHeap_free(MemoryHeap* heap, void* ptr);

int MemoryHeap_destroy(MemoryHeap* heap);

#endif