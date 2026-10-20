#ifndef HASHMAP_H
#define HASHMAP_H

#include "DynamicArray.h"
#include "SDL3/SDL.h"
#include "stddef.h"
#include "stdint.h"

// key type support:
// - type: uint32_t
// - type: uint64_t
// - type: HashMapString

// Examples of Key Value Pair Structs
// typedef struct Uint64KeyExample {
//     int value; // I chose an int here, but the value type can be anything (must have exact name value)
//     uint64_t key; // Must be declared exactly like this
// } Uint64KeyExample;
//
// typedef struct StringKeyExample {
//     int value; // I chose an int here, but the value type can be anything (must have exact name value)
//     char* key; // Must be declared exactly like this
// } StringKeyExample;

// Next, you will have to define a dynamic array of your key value pair type (see notes in DynamicArray.h)
// An example using the Uint64KeyExample is
// typedef struct Uint64KeyValArray {
//     Uint64KeyExample* arr;
//     size_t len;
//     size_t capacity;
// } Uint64KeyValArray;

// Lastly, you will need to define the actual hash map struct, which continuing with the uint64 key
// examples would look like
// typedef struct Uint64HashMap {
//     Uint64KeyValArray* arr;
//     size_t load;
//     size_t capacity;
//     int* ret_val;
// } Uint64HashMap;

typedef struct HashMapInfo {
    void* top_level_array;
    size_t hashmap_capacity;
    size_t bucket_container_size;
    size_t bucket_array_offset;
    size_t bucket_len_offset;
    size_t bucket_element_size;
    size_t key_offset;
    size_t value_offset;
    size_t return_value_offset;
} HashMapInfo;

typedef struct HashMapString {
    char* string;
    size_t len;
} HashMapString;

#define HASH_MAP_STRING_LITERAL(str) (HashMapString){.string = str, .len = sizeof(str) / sizeof(str[0]) - 1}

#define HASH_MAP_STARTING_SIZE 16

// FNV-1a (Fowler-Noll-Vo) hashing function
static inline uint32_t fnv1a_hash(void* key, uint32_t key_len) {
    static const uint32_t FNV_offset_basis = 2166136261;
    static const uint32_t FNV_prime = 16777619;

    uint32_t hash = FNV_offset_basis;

    uint8_t* bytes = (uint8_t*)(key);
    for (uint32_t i = 0; i < key_len; i++) {
        hash ^= bytes[i];
        hash *= FNV_prime;
    }

    return hash;
}

// Supported key_type's are uint32_t, uint64_t, and HashMapString
#define HASH_MAP_DEFINE_TYPE(hashMapName, key_type, value_type) \
    typedef struct hashMapName##_KeyValPair {                   \
        value_type value;                                       \
        key_type key;                                           \
    } hashMapName##_KeyValPair;                                 \
    typedef struct hashMapName##_Bucket {                       \
        hashMapName##_KeyValPair* arr;                          \
        size_t len;                                             \
        size_t capacity;                                        \
    } hashMapName##_Bucket;                                     \
    typedef struct hashMapName {                                \
        hashMapName##_Bucket* arr;                              \
        size_t load;                                            \
        size_t capacity;                                        \
        value_type* ret_val;                                    \
    } hashMapName;

#define HASH_MAP_INIT(hashMap)                                                                                 \
    do {                                                                                                       \
        (hashMap).arr = SDL_malloc(HASH_MAP_STARTING_SIZE * sizeof((hashMap).arr[0]));                         \
        SDL_memset((hashMap).arr, 0, HASH_MAP_STARTING_SIZE * sizeof(hashMap).arr[0]);                         \
        (hashMap).arr == NULL ? SDL_Log("Failed to allocate base memory for hashmap: %s", SDL_GetError()) : 0; \
        (hashMap).capacity = HASH_MAP_STARTING_SIZE;                                                           \
        (hashMap).load = 0;                                                                                    \
    } while (false)

#define HASH_MAP_FREE(hashMap)                            \
    do {                                                  \
        for (size_t i = 0; i < (hashMap).capacity; i++) { \
            DYNAMIC_ARRAY_FREE((hashMap).arr[i]);         \
        }                                                 \
        SDL_free((hashMap).arr);                          \
    } while (false)

static inline void* __hash_map_get_with_uint32_key(HashMapInfo* info, void* hashmap, uint32_t key) {
    uint32_t index = fnv1a_hash(&key, sizeof(uint32_t)) & (info->hashmap_capacity - 1);
    uint8_t* bucket = ((uint8_t*)info->top_level_array) + (index * info->bucket_container_size);

    size_t bucket_len = *(size_t*)(bucket + info->bucket_len_offset);
    uint8_t* bucket_arr = *(uint8_t**)(bucket + info->bucket_array_offset);

    if (bucket_len == 1) {
        void* value_ptr = bucket_arr + info->value_offset;
        void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
        (*return_value) = value_ptr;
        return value_ptr;
    } else if (bucket_len > 1) {
        for (size_t i = 0; i < bucket_len; i++) {
            uint8_t* curr_key_val_pair = bucket_arr + (i * info->bucket_element_size);
            uint32_t curr_key = *(uint32_t*)(curr_key_val_pair + info->key_offset);
            if (curr_key == key) {
                void* value_ptr = curr_key_val_pair + info->value_offset;
                void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
                (*return_value) = value_ptr;
                return value_ptr;
            }
        }
    }

    void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
    (*return_value) = NULL;
    return NULL;
}

static inline void* __hash_map_get_with_uint64_key(HashMapInfo* info, void* hashmap, uint64_t key) {
    uint32_t index = fnv1a_hash(&key, sizeof(uint64_t)) & (info->hashmap_capacity - 1);
    uint8_t* bucket = ((uint8_t*)info->top_level_array) + (index * info->bucket_container_size);

    size_t bucket_len = *(size_t*)(bucket + info->bucket_len_offset);
    uint8_t* bucket_arr = *(uint8_t**)(bucket + info->bucket_array_offset);

    if (bucket_len == 1) {
        void* value_ptr = bucket_arr + info->value_offset;
        void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
        (*return_value) = value_ptr;
        return value_ptr;
    } else if (bucket_len > 1) {
        for (size_t i = 0; i < bucket_len; i++) {
            uint8_t* curr_key_val_pair = bucket_arr + (i * info->bucket_element_size);
            uint64_t curr_key = *(uint64_t*)(curr_key_val_pair + info->key_offset);
            if (curr_key == key) {
                void* value_ptr = curr_key_val_pair + info->value_offset;
                void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
                (*return_value) = value_ptr;
                return value_ptr;
            }
        }
    }

    void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
    (*return_value) = NULL;
    return NULL;
}

static inline void* __hash_map_get_with_string_key(HashMapInfo* info, void* hashmap, HashMapString key) {
    uint32_t index = fnv1a_hash(key.string, key.len) & (info->hashmap_capacity - 1);
    uint8_t* bucket = ((uint8_t*)info->top_level_array) + (index * info->bucket_container_size);

    size_t bucket_len = *(size_t*)(bucket + info->bucket_len_offset);
    uint8_t* bucket_arr = *(uint8_t**)(bucket + info->bucket_array_offset);

    if (bucket_len == 1) {
        void* value_ptr = bucket_arr + info->value_offset;
        void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
        (*return_value) = value_ptr;
        return value_ptr;
    } else if (bucket_len > 1) {
        for (size_t i = 0; i < bucket_len; i++) {
            uint8_t* curr_key_val_pair = bucket_arr + (i * info->bucket_element_size);
            HashMapString curr_key = *(HashMapString*)(curr_key_val_pair + info->key_offset);
            if (curr_key.len == key.len && SDL_memcmp(curr_key.string, key.string, key.len) == 0) {
                void* value_ptr = curr_key_val_pair + info->value_offset;
                void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
                (*return_value) = value_ptr;
                return value_ptr;
            }
        }
    }

    void** return_value = (void**)((uint8_t*)hashmap + info->return_value_offset);
    (*return_value) = NULL;
    return NULL;
}

#define HASH_MAP_INFO(hashMap)                                            \
    ((HashMapInfo){                                                       \
        .top_level_array = (hashMap).arr,                                 \
        .hashmap_capacity = (hashMap).capacity,                           \
        .bucket_container_size = sizeof((hashMap).arr[0]),                \
        .bucket_array_offset = offsetof(typeof((hashMap).arr[0]), arr),   \
        .bucket_len_offset = offsetof(typeof((hashMap).arr[0]), len),     \
        .bucket_element_size = sizeof((hashMap).arr[0].arr[0]),           \
        .key_offset = offsetof(typeof((hashMap).arr[0].arr[0]), key),     \
        .value_offset = offsetof(typeof((hashMap).arr[0].arr[0]), value), \
        .return_value_offset = offsetof(typeof(hashMap), ret_val),        \
    })

#define HASH_MAP_UINT32_GET(hashMap, key) \
    __hash_map_get_with_uint32_key(&HASH_MAP_INFO(hashMap), &(hashMap), key) == NULL ? 0 : (*((hashMap).ret_val))

#define HASH_MAP_UINT64_GET(hashMap, key) \
    __hash_map_get_with_uint64_key(&HASH_MAP_INFO(hashMap), &(hashMap), key) == NULL ? 0 : (*((hashMap).ret_val))

#define HASH_MAP_STRING_GET(hashMap, key) \
    __hash_map_get_with_string_key(&HASH_MAP_INFO(hashMap), &(hashMap), key) == NULL ? (typeof(*((hashMap).ret_val))){0} : (*((hashMap).ret_val))

#define HASH_MAP_UINT32_PUT(hashMap, uint32_key, element)                                                                                              \
    do {                                                                                                                                               \
        typeof((hashMap).arr[0].arr[0].value)* existing_element_ptr = __hash_map_get_with_uint32_key(&HASH_MAP_INFO(hashMap), &(hashMap), uint32_key); \
        if (existing_element_ptr != NULL) {                                                                                                            \
            (*existing_element_ptr) = element;                                                                                                         \
            break;                                                                                                                                     \
        }                                                                                                                                              \
        if ((hashMap).load >= 3 * (hashMap).capacity / 4) {                                                                                            \
            typeof(hashMap) newHashMap;                                                                                                                \
            newHashMap.load = 0;                                                                                                                       \
            newHashMap.capacity = 2 * (hashMap).capacity;                                                                                              \
            newHashMap.arr = SDL_malloc(newHashMap.capacity * sizeof((hashMap).arr[0]));                                                               \
            SDL_memset(newHashMap.arr, 0, newHashMap.capacity * sizeof(newHashMap.arr[0]));                                                            \
            if (newHashMap.arr == NULL) {                                                                                                              \
                SDL_Log("Failed to allocate more memory for hashmap: %s", SDL_GetError());                                                             \
            } else {                                                                                                                                   \
                for (size_t i = 0; i < (hashMap).capacity; i++) {                                                                                      \
                    for (size_t j = 0; j < (hashMap).arr[i].len; j++) {                                                                                \
                        uint32_t index = fnv1a_hash(&(hashMap).arr[i].arr[j].key, sizeof(uint32_t)) & (newHashMap.capacity - 1);                       \
                        typeof(newHashMap.arr[0].arr[0]) keyValStruct;                                                                                 \
                        keyValStruct.value = (hashMap).arr[i].arr[j].value;                                                                            \
                        keyValStruct.key = (hashMap).arr[i].arr[j].key;                                                                                \
                        DYNAMIC_ARRAY_APPEND((newHashMap).arr[index], keyValStruct);                                                                   \
                        newHashMap.load++;                                                                                                             \
                    }                                                                                                                                  \
                }                                                                                                                                      \
                HASH_MAP_FREE(hashMap);                                                                                                                \
                (hashMap) = newHashMap;                                                                                                                \
            }                                                                                                                                          \
        }                                                                                                                                              \
        uint32_t index = fnv1a_hash((uint32_t[1]){uint32_key}, sizeof(uint32_t)) & ((hashMap).capacity - 1);                                           \
        typeof((hashMap).arr[0].arr[0]) keyValStruct;                                                                                                  \
        keyValStruct.value = element;                                                                                                                  \
        keyValStruct.key = uint32_key;                                                                                                                 \
        DYNAMIC_ARRAY_APPEND((hashMap).arr[index], keyValStruct);                                                                                      \
        (hashMap).load++;                                                                                                                              \
    } while (false)

#define HASH_MAP_UINT64_PUT(hashMap, uint64_key, element)                                                                                              \
    do {                                                                                                                                               \
        typeof((hashMap).arr[0].arr[0].value)* existing_element_ptr = __hash_map_get_with_uint64_key(&HASH_MAP_INFO(hashMap), &(hashMap), uint64_key); \
        if (existing_element_ptr != NULL) {                                                                                                            \
            (*existing_element_ptr) = element;                                                                                                         \
            break;                                                                                                                                     \
        }                                                                                                                                              \
        if ((hashMap).load >= 3 * (hashMap).capacity / 4) {                                                                                            \
            typeof(hashMap) newHashMap;                                                                                                                \
            newHashMap.load = 0;                                                                                                                       \
            newHashMap.capacity = 2 * (hashMap).capacity;                                                                                              \
            newHashMap.arr = SDL_malloc(newHashMap.capacity * sizeof((hashMap).arr[0]));                                                               \
            SDL_memset(newHashMap.arr, 0, newHashMap.capacity * sizeof(newHashMap.arr[0]));                                                            \
            if (newHashMap.arr == NULL) {                                                                                                              \
                SDL_Log("Failed to allocate more memory for hashmap: %s", SDL_GetError());                                                             \
            } else {                                                                                                                                   \
                for (size_t i = 0; i < (hashMap).capacity; i++) {                                                                                      \
                    for (size_t j = 0; j < (hashMap).arr[i].len; j++) {                                                                                \
                        uint32_t index = fnv1a_hash(&(hashMap).arr[i].arr[j].key, sizeof(uint64_t)) & (newHashMap.capacity - 1);                       \
                        typeof(newHashMap.arr[0].arr[0]) keyValStruct;                                                                                 \
                        keyValStruct.value = (hashMap).arr[i].arr[j].value;                                                                            \
                        keyValStruct.key = (hashMap).arr[i].arr[j].key;                                                                                \
                        DYNAMIC_ARRAY_APPEND((newHashMap).arr[index], keyValStruct);                                                                   \
                        newHashMap.load++;                                                                                                             \
                    }                                                                                                                                  \
                }                                                                                                                                      \
                HASH_MAP_FREE(hashMap);                                                                                                                \
                (hashMap) = newHashMap;                                                                                                                \
            }                                                                                                                                          \
        }                                                                                                                                              \
        uint32_t index = fnv1a_hash((uint64_t[1]){uint64_key}, sizeof(uint64_t)) & ((hashMap).capacity - 1);                                           \
        typeof((hashMap).arr[0].arr[0]) keyValStruct;                                                                                                  \
        keyValStruct.value = element;                                                                                                                  \
        keyValStruct.key = uint64_key;                                                                                                                 \
        DYNAMIC_ARRAY_APPEND((hashMap).arr[index], keyValStruct);                                                                                      \
        (hashMap).load++;                                                                                                                              \
    } while (false)

#define HASH_MAP_STRING_PUT(hashMap, string_key, element)                                                                                              \
    do {                                                                                                                                               \
        typeof((hashMap).arr[0].arr[0].value)* existing_element_ptr = __hash_map_get_with_string_key(&HASH_MAP_INFO(hashMap), &(hashMap), string_key); \
        if (existing_element_ptr != NULL) {                                                                                                            \
            (*existing_element_ptr) = element;                                                                                                         \
            break;                                                                                                                                     \
        }                                                                                                                                              \
        if ((hashMap).load >= 3 * (hashMap).capacity / 4) {                                                                                            \
            typeof(hashMap) newHashMap;                                                                                                                \
            newHashMap.load = 0;                                                                                                                       \
            newHashMap.capacity = 2 * (hashMap).capacity;                                                                                              \
            newHashMap.arr = SDL_malloc(newHashMap.capacity * sizeof((hashMap).arr[0]));                                                               \
            SDL_memset(newHashMap.arr, 0, newHashMap.capacity * sizeof(newHashMap.arr[0]));                                                            \
            if (newHashMap.arr == NULL) {                                                                                                              \
                SDL_Log("Failed to allocate more memory for hashmap: %s", SDL_GetError());                                                             \
            } else {                                                                                                                                   \
                for (size_t i = 0; i < (hashMap).capacity; i++) {                                                                                      \
                    for (size_t j = 0; j < (hashMap).arr[i].len; j++) {                                                                                \
                        uint32_t index = fnv1a_hash((hashMap).arr[i].arr[j].key.string, (hashMap).arr[i].arr[j].key.len) & (newHashMap.capacity - 1);  \
                        typeof(newHashMap.arr[0].arr[0]) keyValStruct;                                                                                 \
                        keyValStruct.value = (hashMap).arr[i].arr[j].value;                                                                            \
                        keyValStruct.key = (hashMap).arr[i].arr[j].key;                                                                                \
                        DYNAMIC_ARRAY_APPEND((newHashMap).arr[index], keyValStruct);                                                                   \
                        newHashMap.load++;                                                                                                             \
                    }                                                                                                                                  \
                }                                                                                                                                      \
                HASH_MAP_FREE(hashMap);                                                                                                                \
                (hashMap) = newHashMap;                                                                                                                \
            }                                                                                                                                          \
        }                                                                                                                                              \
        uint32_t index = fnv1a_hash(string_key.string, string_key.len) & ((hashMap).capacity - 1);                                                     \
        typeof((hashMap).arr[0].arr[0]) keyValStruct;                                                                                                  \
        keyValStruct.value = element;                                                                                                                  \
        keyValStruct.key = string_key;                                                                                                                 \
        DYNAMIC_ARRAY_APPEND((hashMap).arr[index], keyValStruct);                                                                                      \
        (hashMap).load++;                                                                                                                              \
    } while (false)

#endif