#include "MemoryArena.h"
#include "DynamicArray.h"
#include "SDL3/SDL.h"

#define ALIGN_POINTER(ptr, alignAmount) \
    (uintptr_t)(ptr) & (alignAmount - 1) == 0 ? (ptr) : (typeof(ptr))((uintptr_t)(ptr) - ((uintptr_t)(ptr) & (alignAmount - 1)) + alignAmount)

int MemoryArena_create(MemoryArena* arena, size_t size) {
    DYNAMIC_ARRAY_INIT((*arena));

    if (size < MEMORY_ARENA_BASE_ALIGN) {
        size = MEMORY_ARENA_BASE_ALIGN;
    }

    MemoryArenaBlock block;
    block.base = (uint8_t*)SDL_malloc(size);
    if (block.base == NULL) {
        SDL_Log("Failed to allocate memory for arena: %s", SDL_GetError());
        return -1;
    }
    block.capacity = size;
    block.tracker = block.base;

    DYNAMIC_ARRAY_APPEND((*arena), block);

    return 0;
}

void* MemoryArena_alloc(MemoryArena* arena, size_t size) {
    for (int i = 0; i < arena->len; i++) {
        arena->arr[i].tracker = ALIGN_POINTER(arena->arr[i].tracker, MEMORY_ARENA_BASE_ALIGN);

        if (arena->arr[i].tracker - arena->arr[i].base + size > arena->arr[i].capacity) {
            continue;
        }

        void* ptr = (void*)arena->arr[i].tracker;
        arena->arr[i].tracker += size;

        return ptr;
    }

    // If the function makes it here, that means there was no space available, so allocate a new block
    MemoryArenaBlock new_block = {0};
    // 1.5x should be a good enough multiplier for the memory given that the base capacity of arenas are
    // already supposed to be fairly large
    new_block.capacity = (3 * arena->arr[0].capacity) / 2;
    new_block.base = (uint8_t*)SDL_malloc(new_block.capacity);
    if (new_block.base == NULL) {
        SDL_Log("Failed to allocate memory for arena: %s", SDL_GetError());
        return NULL;
    }
    new_block.tracker = new_block.base;

    new_block.tracker = ALIGN_POINTER(new_block.tracker, MEMORY_ARENA_BASE_ALIGN);
    void* ptr = (void*)new_block.tracker;
    new_block.tracker += size;

    // Must come after all the values for new_block are updated that way the arena
    // stores the most up-to-date information (I ran in to the classic error of appending and then updating
    // the new_block variable instead of the actual data inside the arena array)
    DYNAMIC_ARRAY_APPEND((*arena), new_block);

    return ptr;
}

void* MemoryArena_aligned_alloc(MemoryArena* arena, size_t size, size_t align_to) {
    if (align_to < MEMORY_ARENA_BASE_ALIGN) {
        align_to = MEMORY_ARENA_BASE_ALIGN;
    }

    for (int i = 0; i < arena->len; i++) {
        arena->arr[i].tracker = ALIGN_POINTER(arena->arr[i].tracker, align_to);

        if (arena->arr[i].tracker - arena->arr[i].base + size > arena->arr[i].capacity) {
            continue;
        }

        void* ptr = (void*)arena->arr[i].tracker;
        arena->arr[i].tracker += size;

        return ptr;
    }

    // If the function makes it here, that means there was no space available, so allocate a new block
    MemoryArenaBlock new_block = {0};
    // 1.5x should be a good enough multiplier for the memory given that the base capacity of arenas are
    // already supposed to be fairly large
    new_block.capacity = (3 * arena->arr[0].capacity) / 2;
    new_block.base = (uint8_t*)SDL_malloc(new_block.capacity);
    if (new_block.base == NULL) {
        SDL_Log("Failed to allocate memory for arena: %s", SDL_GetError());
        return NULL;
    }
    new_block.tracker = new_block.base;

    new_block.tracker = ALIGN_POINTER(new_block.tracker, align_to);
    void* ptr = (void*)new_block.tracker;
    new_block.tracker += size;

    // Must come after all the values for new_block are updated that way the arena
    // stores the most up-to-date information (I ran in to the classic error of appending and then updating
    // the new_block variable instead of the actual data inside the arena array)
    DYNAMIC_ARRAY_APPEND((*arena), new_block);

    return ptr;
}

void MemoryArena_clear(MemoryArena* arena) {
    for (int i = 0; i < arena->len; i++) {
        arena->arr[i].tracker = arena->arr[i].base;
    }
}

int MemoryArena_destroy(MemoryArena* arena) {
    for (int i = 0; i < arena->len; i++) {
        SDL_free(arena->arr[i].base);
    }
    DYNAMIC_ARRAY_FREE((*arena));
    return 0;
}