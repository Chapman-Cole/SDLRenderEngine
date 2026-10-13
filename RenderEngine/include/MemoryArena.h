#ifndef MEMORYARENA_H
#define MEMORYARENA_H

#include "stdint.h"

typedef struct MemoryArenaBlock {
    void* base;
    uint8_t* tracker;
    size_t amountAllocated;
    size_t capacity;
} MemoryArenaBlock;

typedef struct MemoryArena {
    MemoryArenaBlock* arr;
    size_t len;
    size_t capacity;
} MemoryArena;

int MemoryArena_create(MemoryArena* arena, size_t size);

// This is just a very basic linear allocator
void* MemoryArena_allocate(MemoryArena* arena, size_t size);

// Frees all of the memory allocated using MemoryArena_allocate
// After calling this function, new memory may be allocated with the same arena, but you must not
// reference previous allocations
void MemoryArena_clear(MemoryArena* arena);

int MemoryArena_destroy(MemoryArena* arena);

#endif