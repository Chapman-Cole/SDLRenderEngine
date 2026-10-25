#ifndef HASHMAP_H
#define HASHMAP_H

#include "DynamicArray.h"
#include "MemoryHeap.h"
#include "SDL3/SDL.h"
#include "stddef.h"
#include "stdint.h"

// All of the commented out elements are placed at the start of their respective struct, meaning their offset
// is gaurenteed to be 0 and is therefore redundant information
typedef struct HashMapInfo {
    // uint32_t value_offset;
    uint32_t key_offset;
    // uint32_t key_val_pair_array_offset;
    uint32_t key_val_pair_size;
    uint32_t key_val_pairs_len_offset;
    uint32_t key_val_pairs_capacity_offset;
    // uint32_t hashmap_bucket_array_offset;
    uint32_t hashmap_bucket_size;
    uint32_t hashmap_load_offset;
    uint32_t hashmap_capacity_offset;
    uint32_t hashmap_ret_val_offset;
} HashMapInfo;

#define HASH_MAP_INFO(hashMap)                                                         \
    ((HashMapInfo){                                                                    \
        .key_offset = offsetof(typeof((hashMap).arr[0].arr[0]), key),                  \
        .key_val_pair_size = sizeof((hashMap).arr[0].arr[0]),                          \
        .key_val_pairs_len_offset = offsetof(typeof((hashMap).arr[0]), len),           \
        .key_val_pairs_capacity_offset = offsetof(typeof((hashMap).arr[0]), capacity), \
        .hashmap_bucket_size = sizeof((hashMap).arr[0]),                               \
        .hashmap_load_offset = offsetof(typeof(hashMap), load),                        \
        .hashmap_capacity_offset = offsetof(typeof(hashMap), capacity),                \
        .hashmap_ret_val_offset = offsetof(typeof(hashMap), ret_val),                  \
    })

typedef struct HashMapString {
    char* string;
    size_t len;
} HashMapString;

#define HASH_MAP_STRING(str, len) ((HashMapString){.string = str, .len = len})
#define HASH_MAP_STRING_FROM_LITERAL(str) ((HashMapString){.string = str, .len = sizeof(str) / sizeof(str[0]) - 1})
#define HASH_MAP_STRING_FROM_CSTRING(str) ((HashMapString){.string = str, .len = SDL_strlen(str)})

#define HASH_MAP_STARTING_SIZE 16

#define HASH_MAP_STRING_ARENA_STARTING_SIZE 1'000

// key type support:
// - type: uint32_t
// - type: uint64_t
// - type: HashMapString

#define HASH_MAP_UINT32_KEY_DEFINE_TYPE(hashMapName, value_type) \
    typedef struct hashMapName##_KeyValPair {                    \
        value_type value;                                        \
        uint32_t key;                                            \
    } hashMapName##_KeyValPair;                                  \
    typedef struct hashMapName##_Bucket {                        \
        hashMapName##_KeyValPair* arr;                           \
        size_t len;                                              \
        size_t capacity;                                         \
    } hashMapName##_Bucket;                                      \
    typedef struct hashMapName {                                 \
        hashMapName##_Bucket* arr;                               \
        size_t load;                                             \
        size_t capacity;                                         \
        value_type* ret_val;                                     \
    } hashMapName;

#define HASH_MAP_UINT64_KEY_DEFINE_TYPE(hashMapName, value_type) \
    typedef struct hashMapName##_KeyValPair {                    \
        value_type value;                                        \
        uint64_t key;                                            \
    } hashMapName##_KeyValPair;                                  \
    typedef struct hashMapName##_Bucket {                        \
        hashMapName##_KeyValPair* arr;                           \
        size_t len;                                              \
        size_t capacity;                                         \
    } hashMapName##_Bucket;                                      \
    typedef struct hashMapName {                                 \
        hashMapName##_Bucket* arr;                               \
        size_t load;                                             \
        size_t capacity;                                         \
        value_type* ret_val;                                     \
    } hashMapName;

// The hash map will take ownership of string keys so that it
// ensures they last the lifetime of the hashmap
#define HASH_MAP_STRING_KEY_DEFINE_TYPE(hashMapName, value_type) \
    typedef struct hashMapName##_KeyValPair {                    \
        value_type value;                                        \
        HashMapString key;                                       \
    } hashMapName##_KeyValPair;                                  \
    typedef struct hashMapName##_Bucket {                        \
        hashMapName##_KeyValPair* arr;                           \
        size_t len;                                              \
        size_t capacity;                                         \
    } hashMapName##_Bucket;                                      \
    typedef struct hashMapName {                                 \
        hashMapName##_Bucket* arr;                               \
        size_t load;                                             \
        size_t capacity;                                         \
        value_type* ret_val;                                     \
        MemoryHeap heap;                                         \
    } hashMapName;

#define __HASH_MAP_INIT(hashMap)                                                                               \
    do {                                                                                                       \
        (hashMap).arr = SDL_malloc(HASH_MAP_STARTING_SIZE * sizeof((hashMap).arr[0]));                         \
        SDL_memset((hashMap).arr, 0, HASH_MAP_STARTING_SIZE * sizeof(hashMap).arr[0]);                         \
        (hashMap).arr == NULL ? SDL_Log("Failed to allocate base memory for hashmap: %s", SDL_GetError()) : 0; \
        (hashMap).capacity = HASH_MAP_STARTING_SIZE;                                                           \
        (hashMap).load = 0;                                                                                    \
    } while (false)

#define HASH_MAP_UINT32_INIT(hashMap) \
    __HASH_MAP_INIT(hashMap)

#define HASH_MAP_UINT64_INIT(hashMap) \
    __HASH_MAP_INIT(hashMap)

// For hashmaps that will store a large amount of strings, a larger
// heap size should be used
#define HASH_MAP_STRING_INIT(hashMap, heapSize)         \
    do {                                                \
        __HASH_MAP_INIT(hashMap);                       \
        MemoryHeap_create(&((hashMap).heap), heapSize); \
    } while (false)

#define __HASH_MAP_FREE(hashMap)                          \
    do {                                                  \
        for (size_t i = 0; i < (hashMap).capacity; i++) { \
            DYNAMIC_ARRAY_FREE((hashMap).arr[i]);         \
        }                                                 \
        SDL_free((hashMap).arr);                          \
    } while (false)

#define HASH_MAP_UINT32_FREE(hashMap) \
    __HASH_MAP_FREE(hashMap)

#define HASH_MAP_UINT64_FREE(hashMap) \
    __HASH_MAP_FREE(hashMap)

#define HASH_MAP_STRING_FREE(hashMap)          \
    do {                                       \
        __HASH_MAP_FREE(hashMap);              \
        MemoryHeap_destroy(&((hashMap).heap)); \
    } while (false)

void* __hash_map_get_with_uint32_key(HashMapInfo* info, void* hashmap, uint32_t key);

void* __hash_map_get_with_uint64_key(HashMapInfo* info, void* hashmap, uint64_t key);

void* __hash_map_get_with_string_key(HashMapInfo* info, void* hashmap, HashMapString key);

#define HASH_MAP_UINT32_GET(hashMap, key) \
    (__hash_map_get_with_uint32_key(&HASH_MAP_INFO(hashMap), &(hashMap), key) == NULL ? (typeof(*((hashMap).ret_val))){0} : (*((hashMap).ret_val)))

#define HASH_MAP_UINT64_GET(hashMap, key) \
    (__hash_map_get_with_uint64_key(&HASH_MAP_INFO(hashMap), &(hashMap), key) == NULL ? (typeof(*((hashMap).ret_val))){0} : (*((hashMap).ret_val)))

#define HASH_MAP_STRING_GET(hashMap, key) \
    (__hash_map_get_with_string_key(&HASH_MAP_INFO(hashMap), &(hashMap), key) == NULL ? (typeof(*((hashMap).ret_val))){0} : (*((hashMap).ret_val)))

int __hash_map_put_element_with_uint32_key(HashMapInfo* info, void* hashmap, uint32_t uint32_key, void* element, size_t element_size);

int __hash_map_put_element_with_uint64_key(HashMapInfo* info, void* hashmap, uint64_t uint64_key, void* element, size_t element_size);

int __hash_map_put_element_with_string_key(HashMapInfo* info, void* hashmap, HashMapString string_key, void* element, size_t element_size, MemoryHeap* memoryHeap);

#define HASH_MAP_UINT32_PUT(hashMap, uint32_key, element) \
    __hash_map_put_element_with_uint32_key(&HASH_MAP_INFO(hashMap), &(hashMap), uint32_key, (typeof (*((hashMap).ret_val))[1]){element}, sizeof(*((hashMap).ret_val)))

#define HASH_MAP_UINT64_PUT(hashMap, uint64_key, element) \
    __hash_map_put_element_with_uint64_key(&HASH_MAP_INFO(hashMap), &(hashMap), uint64_key, (typeof (*((hashMap).ret_val))[1]){element}, sizeof(*((hashMap).ret_val)))

#define HASH_MAP_STRING_PUT(hashMap, string_key, element) \
    __hash_map_put_element_with_string_key(&HASH_MAP_INFO(hashMap), &(hashMap), string_key, (typeof (*((hashMap).ret_val))[1]){element}, sizeof(*((hashMap).ret_val)), &((hashMap).heap))

#endif