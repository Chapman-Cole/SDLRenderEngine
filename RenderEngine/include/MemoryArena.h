#ifndef MEMORYARENA_H
#define MEMORYARENA_H

#include <stdint.h>
#include <stddef.h>

// The alignment amount on x64 is 16 bytes by default (for malloc), which is technically more than is necessary for some other platforms,
// but since 16 byte alignment also gaurentees 8, 4, 2, and of course 1 byte alignment it is not really an issu
#define MEMORY_ARENA_BASE_ALIGN 16

typedef struct MemoryArenaBlock {
    uint8_t* base;
    uint8_t* tracker;
    size_t capacity;
} MemoryArenaBlock;

typedef struct MemoryArena {
    MemoryArenaBlock* arr;
    size_t len;
    size_t capacity;
} MemoryArena;

int MemoryArena_create(MemoryArena* arena, size_t size);

// This is just a very basic linear allocator
void* MemoryArena_alloc(MemoryArena* arena, size_t size);

void* MemoryArena_aligned_alloc(MemoryArena* arena, size_t size, size_t align_to);

// Frees all of the memory allocated using MemoryArena_allocate
// After calling this function, new memory may be allocated with the same arena, but you must not
// reference previous allocations
void MemoryArena_clear(MemoryArena* arena);

int MemoryArena_destroy(MemoryArena* arena);

#endif