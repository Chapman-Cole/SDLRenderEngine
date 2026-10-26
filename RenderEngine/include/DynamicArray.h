#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include "SDL3/SDL.h"
#include "stddef.h"
#include "stdint.h"

// Instructions:
// When you pass in a struct to the macros, the struct you have defined must include
// a pointer type (can be any pointer type) with the exact name 'arr', a size_t type with the exact name 'len',
// and a size_t type with the exact name 'capacity'. An example of a dynamic array struct for integers would look like
// typedef struct IntegerArray {
//     int* arr;
//     size_t len;
//     size_t capacity;
// } IntegerArray;
// The macros then use the fact that the structs have these predefined, named variables to update the length
// and capacity accordingly, set elements in the array, etc.

// Specifies the offsets (in bytes) to the array pointer, len, and capacity of the array in
// the struct. The element_size will be provided directly
typedef struct DynamicArrayInfo {
    size_t arr;
    size_t len;
    size_t capacity;
    size_t element_size;
} DynamicArrayInfo;

static inline int _dynamic_array_grow(void* arrayStruct, DynamicArrayInfo* arrayInfo, size_t amount) {
    size_t* len = (size_t*)((uint8_t*)arrayStruct + arrayInfo->len);
    size_t* capacity = (size_t*)((uint8_t*)arrayStruct + arrayInfo->capacity);
    void** arr = (void**)((uint8_t*)arrayStruct + arrayInfo->arr);

    if (*len + amount >= *capacity) {
        *capacity *= 2;
        // For small inputs, simply doubling may not be enough to cover the amount being expanded by
        if (*capacity < *len + amount) {
            *capacity += amount;
        }

        void* temp_ptr = SDL_realloc(*arr, (*capacity) * arrayInfo->element_size);
        if (temp_ptr == NULL) {
            return false;
        }
        *arr = temp_ptr;
    }

    return true;
}

static inline int _dynamic_array_shrink(void* arrayStruct, DynamicArrayInfo* arrayInfo, size_t amount) {
    size_t* len = (size_t*)((uint8_t*)arrayStruct + arrayInfo->len);
    size_t* capacity = (size_t*)((uint8_t*)arrayStruct + arrayInfo->capacity);
    void** arr = (void**)((uint8_t*)arrayStruct + arrayInfo->arr);

    if (*len - amount < *capacity / 2) {
        *capacity /= 2;
        if (*capacity == 0) {
            *capacity = 1;
        }

        void* temp_ptr = SDL_realloc(*arr, *capacity * arrayInfo->element_size);
        if (temp_ptr == NULL) {
            return false;
        }
        *arr = temp_ptr;
    }

    return true;
}

#define DYNAMIC_ARRAY_INIT(arrayStruct) \
    (arrayStruct).arr = NULL;           \
    (arrayStruct).len = 0;              \
    (arrayStruct).capacity = 1;

#define DYNAMIC_ARRAY_INFO(arrayStruct)                        \
    ((DynamicArrayInfo){                                       \
        .arr = offsetof(typeof((arrayStruct)), arr),           \
        .len = offsetof(typeof((arrayStruct)), len),           \
        .capacity = offsetof(typeof((arrayStruct)), capacity), \
        .element_size = sizeof(*((arrayStruct).arr)),          \
    })

#define DYNAMIC_ARRAY_FREE(arrayStruct) \
    SDL_free((arrayStruct).arr);        \
    (arrayStruct).arr = NULL;           \
    (arrayStruct).len = 0;              \
    (arrayStruct).capacity = 1;

// When appending a struct element, you will have to wrap inline declarations in parentheses
#define DYNAMIC_ARRAY_APPEND(arrayStruct, element)                              \
    _dynamic_array_grow(&(arrayStruct), &DYNAMIC_ARRAY_INFO((arrayStruct)), 1); \
    arrayStruct.arr[arrayStruct.len] = (element);                               \
    arrayStruct.len++;

#define DYNAMIC_ARRAY_POP(arrayStruct)                                            \
    _dynamic_array_shrink(&(arrayStruct), &DYNAMIC_ARRAY_INFO((arrayStruct)), 1); \
    (arrayStruct).len -= (arrayStruct).len == 0 ? 0 : 1;

// When inserting a struct element, you will have to wrap inline declarations in parentheses
#define DYNAMIC_ARRAY_INSERT(arrayStruct, index, element)                                                                                        \
    _dynamic_array_grow(&(arrayStruct), &DYNAMIC_ARRAY_INFO((arrayStruct)), 1);                                                                  \
    SDL_memmove(&((arrayStruct).arr[(index) + 1]), &((arrayStruct).arr[(index)]), ((arrayStruct).len - (index)) * sizeof(*((arrayStruct).arr))); \
    (arrayStruct).arr[(index)] = (element);                                                                                                      \
    (arrayStruct).len++;

// For memory removal operations it is very important to only shrink the array after moving the data around, that way no index out of bounds
// errors occur or any data is lost upon the resizing
#define DYNAMIC_ARRAY_REMOVE(arrayStruct, index)                                                                                                     \
    SDL_memmove(&((arrayStruct).arr[(index)]), &((arrayStruct).arr[(index) + 1]), ((arrayStruct).len - (index) - 1) * sizeof(*((arrayStruct).arr))); \
    _dynamic_array_shrink(&(arrayStruct), &DYNAMIC_ARRAY_INFO((arrayStruct)), 1);                                                                    \
    (arrayStruct).len -= ((arrayStruct).len == 0 ? 0 : 1);

// For memory removal operations it is very important to only shrink the array after moving the data around, that way no index out of bounds
// errors occur or any data is lost upon the resizing
#define DYNAMIC_ARRAY_REMOVE_SELECTION(arrayStruct, start, selection_len)                                                                                                          \
    SDL_memmove(&((arrayStruct).arr[(start)]), &((arrayStruct).arr[(start) + (selection_len)]), ((arrayStruct).len - ((start) + (selection_len))) * sizeof(*((arrayStruct).arr))); \
    _dynamic_array_shrink(&(arrayStruct), &DYNAMIC_ARRAY_INFO((arrayStruct)), selection_len);                                                                                      \
    (arrayStruct).len -= ((arrayStruct).len < (selection_len) ? 0 : (selection_len));

// When declaring an inline struct for the src struct, you will have to wrap it in parentheses
// the dst struct should not be declared inline since it is being directly modified
#define DYNAMIC_ARRAY_INSERT_ARRAY(dst, src, index)                                                                      \
    _dynamic_array_grow(&(dst), &DYNAMIC_ARRAY_INFO((dst)), (src).len);                                                  \
    SDL_memmove(&((dst).arr[(index) + (src).len]), &((dst).arr[(index)]), ((dst).len - (index)) * sizeof(*((dst).arr))); \
    SDL_memcpy(&((dst).arr[(index)]), (src).arr, (src).len * sizeof(*((src).arr)));                                      \
    (dst).len += (src).len;

// Useful for when preserving the buffer is desired (typically for performance reasons since freeing and allocating memory is slow)
// but the dynamic buffer needs to start appending elements from the beginning of the array
#define DYNAMIC_ARRAY_CLEAR(arrayStruct) (arrayStruct).len = 0;

#endif