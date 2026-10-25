#include "HashMap.h"

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

void* __hash_map_get_with_uint32_key(HashMapInfo* info, void* hashmap, uint32_t key) {
    size_t hashmap_capacity = *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset);
    uint8_t* hashmap_bucket_array = *(uint8_t**)(hashmap); // The bucket array is the first element, so it has offset 0 always

    uint32_t index = fnv1a_hash(&key, sizeof(uint32_t)) & (hashmap_capacity - 1);
    uint8_t* bucket = hashmap_bucket_array + (index * info->hashmap_bucket_size);

    size_t key_val_pairs_len = *(size_t*)(bucket + info->key_val_pairs_len_offset);
    uint8_t* key_val_pairs = *(uint8_t**)(bucket); // The offset of the array for the key value pairs is always 0 since it's the first element

    for (size_t i = 0; i < key_val_pairs_len; i++) {
        uint8_t* curr_key_val_pair = key_val_pairs + (i * info->key_val_pair_size);
        uint32_t curr_key = *(uint32_t*)(curr_key_val_pair + info->key_offset);
        if (curr_key == key) {
            void* value_ptr = curr_key_val_pair; // The value is the first element in its respective struct, so it has an offset of 0
            void** return_value = (void**)((uint8_t*)hashmap + info->hashmap_ret_val_offset);
            (*return_value) = value_ptr;
            return value_ptr;
        }
    }

    SDL_assert("Invalid hashmap key. Returning NULL ptr.\n");
    void** return_value = (void**)((uint8_t*)hashmap + info->hashmap_ret_val_offset);
    (*return_value) = NULL;
    return NULL;
}

void* __hash_map_get_with_uint64_key(HashMapInfo* info, void* hashmap, uint64_t key) {
    size_t hashmap_capacity = *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset);
    uint8_t* hashmap_bucket_array = *(uint8_t**)(hashmap); // Bucket is the first element in the struct, so offset is always 0

    uint32_t index = fnv1a_hash(&key, sizeof(uint64_t)) & (hashmap_capacity - 1);
    uint8_t* bucket = hashmap_bucket_array + (index * info->hashmap_bucket_size);

    size_t key_val_pairs_len = *(size_t*)(bucket + info->key_val_pairs_len_offset);
    uint8_t* key_val_pairs = *(uint8_t**)(bucket); // The array offset in the bucket is 0 since it's the first element

    for (size_t i = 0; i < key_val_pairs_len; i++) {
        uint8_t* curr_key_val_pair = key_val_pairs + (i * info->key_val_pair_size);
        uint64_t curr_key = *(uint64_t*)(curr_key_val_pair + info->key_offset);
        if (curr_key == key) {
            void* value_ptr = curr_key_val_pair; // The value is the first element in its respective struct, so it has an offset of 0
            void** return_value = (void**)((uint8_t*)hashmap + info->hashmap_ret_val_offset);
            (*return_value) = value_ptr;
            return value_ptr;
        }
    }

    void** return_value = (void**)((uint8_t*)hashmap + info->hashmap_ret_val_offset);
    (*return_value) = NULL;
    return NULL;
}

void* __hash_map_get_with_string_key(HashMapInfo* info, void* hashmap, HashMapString key) {
    size_t hashmap_capacity = *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset);
    uint8_t* hashmap_bucket_array = *(uint8_t**)(hashmap); // Bucket is the first element in the struct, so offset is always 0

    uint32_t index = fnv1a_hash(key.string, key.len) & (hashmap_capacity - 1);
    uint8_t* bucket = hashmap_bucket_array + (index * info->hashmap_bucket_size);

    size_t key_val_pairs_len = *(size_t*)(bucket + info->key_val_pairs_len_offset);
    uint8_t* key_val_pairs = *(uint8_t**)(bucket); // The array offset in the bucket is 0 since it's the first element

    for (size_t i = 0; i < key_val_pairs_len; i++) {
        uint8_t* curr_key_val_pair = key_val_pairs + (i * info->key_val_pair_size);
        HashMapString curr_key = *(HashMapString*)(curr_key_val_pair + info->key_offset);
        if (curr_key.len == key.len && SDL_memcmp(curr_key.string, key.string, key.len) == 0) {
            void* value_ptr = curr_key_val_pair; // The value is the first element in its respective struct, so it has an offset of 0
            void** return_value = (void**)((uint8_t*)hashmap + info->hashmap_ret_val_offset);
            (*return_value) = value_ptr;
            return value_ptr;
        }
    }

    void** return_value = (void**)((uint8_t*)hashmap + info->hashmap_ret_val_offset);
    (*return_value) = NULL;
    return NULL;
}

int __hash_map_put_element_with_uint32_key(HashMapInfo* info, void* hashmap, uint32_t uint32_key, void* element, size_t element_size) {
    void* existing_element_ptr = __hash_map_get_with_uint32_key(info, hashmap, uint32_key);
    if (existing_element_ptr != NULL) {
        SDL_memcpy(existing_element_ptr, element, element_size);
        return true;
    }

    uint8_t* hashmap_bucket_array = *(uint8_t**)(hashmap); // The bucket array is the first element, so it has offset 0 always

    size_t hashmap_load = *(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset);
    size_t hashmap_capacity = *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset);
    if (hashmap_load >= 3 * hashmap_capacity / 4) {
        size_t newLoad = 0;
        size_t newCapacity = 2 * hashmap_capacity;
        void* newArray = SDL_malloc(newCapacity * info->hashmap_bucket_size);
        if (newArray == NULL) {
            SDL_Log("Failed to allocate more memory for hashmap: %s", SDL_GetError());
        } else {
            SDL_memset(newArray, 0, newCapacity * info->hashmap_bucket_size);
            for (size_t i = 0; i < hashmap_capacity; i++) {
                uint8_t* curr_bucket = hashmap_bucket_array + (i * info->hashmap_bucket_size);
                size_t curr_bucket_len = *(size_t*)(curr_bucket + info->key_val_pairs_len_offset);
                uint8_t* curr_key_val_pair_arr = *(uint8_t**)(curr_bucket);

                for (size_t j = 0; j < curr_bucket_len; j++) {
                    uint8_t* curr_key_val_pair = curr_key_val_pair_arr + (j * info->key_val_pair_size);
                    uint8_t* curr_key = curr_key_val_pair + info->key_offset;

                    uint32_t index = fnv1a_hash(curr_key, sizeof(uint32_t)) & (newCapacity - 1);
                    uint8_t* new_array_curr_bucket = (uint8_t*)newArray + (index * info->hashmap_bucket_size);

                    _dynamic_array_grow(
                        new_array_curr_bucket,
                        &(DynamicArrayInfo){
                            .arr = 0,
                            .len = info->key_val_pairs_len_offset,
                            .capacity = info->key_val_pairs_capacity_offset,
                            .element_size = info->key_val_pair_size},
                        1);

                    size_t* new_array_curr_bucket_len = (size_t*)(new_array_curr_bucket + info->key_val_pairs_len_offset);
                    uint8_t* new_array_curr_bucket_key_val_pair_array = *(uint8_t**)(new_array_curr_bucket);

                    uint8_t* new_array_curr_key_val_pair = new_array_curr_bucket_key_val_pair_array + ((*new_array_curr_bucket_len) * info->key_val_pair_size);

                    SDL_memcpy(new_array_curr_key_val_pair, curr_key_val_pair, info->key_val_pair_size);
                    (*new_array_curr_bucket_len)++;
                    newLoad++;
                }
                
                SDL_free(curr_key_val_pair_arr);
            }
            SDL_free(hashmap_bucket_array);
            *((uint8_t**)hashmap) = newArray;
            *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset) = newCapacity;
            *(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset) = newLoad;

            // Make sure the local variables are updated to reflect the new changes so it doesn't break the code
            // at the bottom of this function that actually adds the new element
            hashmap_capacity = newCapacity;
            hashmap_bucket_array = newArray;
        }
    }

    uint32_t index = fnv1a_hash(&uint32_key, sizeof(uint32_t)) & (hashmap_capacity - 1);
    uint8_t* curr_bucket = hashmap_bucket_array + (index * info->hashmap_bucket_size);
    _dynamic_array_grow(
        curr_bucket,
        &(DynamicArrayInfo){
            .arr = 0,
            .len = info->key_val_pairs_len_offset,
            .capacity = info->key_val_pairs_capacity_offset,
            .element_size = info->key_val_pair_size,
        },
        1);

    size_t* curr_bucket_len = (size_t*)(curr_bucket + info->key_val_pairs_len_offset);
    uint8_t* curr_bucket_key_val_pair_array = *(uint8_t**)(curr_bucket);

    uint8_t* curr_key_val_pair = curr_bucket_key_val_pair_array + ((*curr_bucket_len) * info->key_val_pair_size);
    SDL_memcpy(curr_key_val_pair, element, element_size); // The element offset is 0, so can just write element data directly to start of address
    SDL_memcpy(curr_key_val_pair + info->key_offset, &uint32_key, sizeof(uint32_t));

    (*curr_bucket_len)++;
    (*(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset))++;

    return true;
}

int __hash_map_put_element_with_uint64_key(HashMapInfo* info, void* hashmap, uint64_t uint64_key, void* element, size_t element_size) {
    void* existing_element_ptr = __hash_map_get_with_uint64_key(info, hashmap, uint64_key);
    if (existing_element_ptr != NULL) {
        SDL_memcpy(existing_element_ptr, element, element_size);
        return true;
    }

    uint8_t* hashmap_bucket_array = *(uint8_t**)(hashmap); // The bucket array is the first element, so it has offset 0 always

    size_t hashmap_load = *(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset);
    size_t hashmap_capacity = *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset);
    if (hashmap_load >= 3 * hashmap_capacity / 4) {
        size_t newLoad = 0;
        size_t newCapacity = 2 * hashmap_capacity;
        void* newArray = SDL_malloc(newCapacity * info->hashmap_bucket_size);
        if (newArray == NULL) {
            SDL_Log("Failed to allocate more memory for hashmap: %s", SDL_GetError());
        } else {
            SDL_memset(newArray, 0, newCapacity * info->hashmap_bucket_size);
            for (size_t i = 0; i < hashmap_capacity; i++) {
                uint8_t* curr_bucket = hashmap_bucket_array + (i * info->hashmap_bucket_size);
                size_t curr_bucket_len = *(size_t*)(curr_bucket + info->key_val_pairs_len_offset);
                uint8_t* curr_key_val_pair_arr = *(uint8_t**)(curr_bucket);

                for (size_t j = 0; j < curr_bucket_len; j++) {
                    uint8_t* curr_key_val_pair = curr_key_val_pair_arr + (j * info->key_val_pair_size);
                    uint8_t* curr_key = curr_key_val_pair + info->key_offset;

                    uint32_t index = fnv1a_hash(curr_key, sizeof(uint64_t)) & (newCapacity - 1);
                    uint8_t* new_array_curr_bucket = (uint8_t*)newArray + (index * info->hashmap_bucket_size);

                    _dynamic_array_grow(
                        new_array_curr_bucket,
                        &(DynamicArrayInfo){
                            .arr = 0,
                            .len = info->key_val_pairs_len_offset,
                            .capacity = info->key_val_pairs_capacity_offset,
                            .element_size = info->key_val_pair_size},
                        1);

                    size_t* new_array_curr_bucket_len = (size_t*)(new_array_curr_bucket + info->key_val_pairs_len_offset);
                    uint8_t* new_array_curr_bucket_key_val_pair_array = *(uint8_t**)(new_array_curr_bucket);

                    uint8_t* new_array_curr_key_val_pair = new_array_curr_bucket_key_val_pair_array + ((*new_array_curr_bucket_len) * info->key_val_pair_size);

                    SDL_memcpy(new_array_curr_key_val_pair, curr_key_val_pair, info->key_val_pair_size);
                    (*new_array_curr_bucket_len)++;
                    newLoad++;
                }
                
                SDL_free(curr_key_val_pair_arr);
            }
            SDL_free(hashmap_bucket_array);
            *((uint8_t**)hashmap) = newArray;
            *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset) = newCapacity;
            *(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset) = newLoad;

            // Make sure the local variables are updated to reflect the new changes so it doesn't break the code
            // at the bottom of this function that actually adds the new element
            hashmap_capacity = newCapacity;
            hashmap_bucket_array = newArray;
        }
    }

    uint32_t index = fnv1a_hash(&uint64_key, sizeof(uint64_t)) & (hashmap_capacity - 1);
    uint8_t* curr_bucket = hashmap_bucket_array + (index * info->hashmap_bucket_size);
    _dynamic_array_grow(
        curr_bucket,
        &(DynamicArrayInfo){
            .arr = 0,
            .len = info->key_val_pairs_len_offset,
            .capacity = info->key_val_pairs_capacity_offset,
            .element_size = info->key_val_pair_size,
        },
        1);

    size_t* curr_bucket_len = (size_t*)(curr_bucket + info->key_val_pairs_len_offset);
    uint8_t* curr_bucket_key_val_pair_array = *(uint8_t**)(curr_bucket);

    uint8_t* curr_key_val_pair = curr_bucket_key_val_pair_array + ((*curr_bucket_len) * info->key_val_pair_size);
    SDL_memcpy(curr_key_val_pair, element, element_size); // The element offset is 0, so can just write element data directly to start of address
    SDL_memcpy(curr_key_val_pair + info->key_offset, &uint64_key, sizeof(uint64_t));

    (*curr_bucket_len)++;
    (*(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset))++;

    return true;
}

int __hash_map_put_element_with_string_key(HashMapInfo* info, void* hashmap, HashMapString string_key, void* element, size_t element_size, MemoryHeap* memoryHeap) {
    void* existing_element_ptr = __hash_map_get_with_string_key(info, hashmap, string_key);
    if (existing_element_ptr != NULL) {
        SDL_memcpy(existing_element_ptr, element, element_size);
        return true;
    }

    uint8_t* hashmap_bucket_array = *(uint8_t**)(hashmap); // The bucket array is the first element, so it has offset 0 always

    size_t hashmap_load = *(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset);
    size_t hashmap_capacity = *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset);
    if (hashmap_load >= 3 * hashmap_capacity / 4) {
        size_t newLoad = 0;
        size_t newCapacity = 2 * hashmap_capacity;
        void* newArray = SDL_malloc(newCapacity * info->hashmap_bucket_size);
        if (newArray == NULL) {
            SDL_Log("Failed to allocate more memory for hashmap: %s", SDL_GetError());
        } else {
            SDL_memset(newArray, 0, newCapacity * info->hashmap_bucket_size);
            for (size_t i = 0; i < hashmap_capacity; i++) {
                uint8_t* curr_bucket = hashmap_bucket_array + (i * info->hashmap_bucket_size);
                size_t curr_bucket_len = *(size_t*)(curr_bucket + info->key_val_pairs_len_offset);
                uint8_t* curr_key_val_pair_arr = *(uint8_t**)(curr_bucket);

                for (size_t j = 0; j < curr_bucket_len; j++) {
                    uint8_t* curr_key_val_pair = curr_key_val_pair_arr + (j * info->key_val_pair_size);
                    HashMapString* curr_key = (HashMapString*)(curr_key_val_pair + info->key_offset);

                    uint32_t index = fnv1a_hash(curr_key->string, curr_key->len) & (newCapacity - 1);
                    uint8_t* new_array_curr_bucket = (uint8_t*)newArray + (index * info->hashmap_bucket_size);

                    _dynamic_array_grow(
                        new_array_curr_bucket,
                        &(DynamicArrayInfo){
                            .arr = 0,
                            .len = info->key_val_pairs_len_offset,
                            .capacity = info->key_val_pairs_capacity_offset,
                            .element_size = info->key_val_pair_size},
                        1);

                    size_t* new_array_curr_bucket_len = (size_t*)(new_array_curr_bucket + info->key_val_pairs_len_offset);
                    uint8_t* new_array_curr_bucket_key_val_pair_array = *(uint8_t**)(new_array_curr_bucket);

                    uint8_t* new_array_curr_key_val_pair = new_array_curr_bucket_key_val_pair_array + ((*new_array_curr_bucket_len) * info->key_val_pair_size);

                    SDL_memcpy(new_array_curr_key_val_pair, curr_key_val_pair, info->key_val_pair_size);
                    (*new_array_curr_bucket_len)++;
                    newLoad++;
                }
                
                SDL_free(curr_key_val_pair_arr);
            }
            SDL_free(hashmap_bucket_array);
            *((uint8_t**)hashmap) = newArray;
            *(size_t*)((uint8_t*)hashmap + info->hashmap_capacity_offset) = newCapacity;
            *(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset) = newLoad;

            // Make sure the local variables are updated to reflect the new changes so it doesn't break the code
            // at the bottom of this function that actually adds the new element
            hashmap_capacity = newCapacity;
            hashmap_bucket_array = newArray;
        }
    }

    uint32_t index = fnv1a_hash(string_key.string, string_key.len) & (hashmap_capacity - 1);
    uint8_t* curr_bucket = hashmap_bucket_array + (index * info->hashmap_bucket_size);
    _dynamic_array_grow(
        curr_bucket,
        &(DynamicArrayInfo){
            .arr = 0,
            .len = info->key_val_pairs_len_offset,
            .capacity = info->key_val_pairs_capacity_offset,
            .element_size = info->key_val_pair_size,
        },
        1);

    size_t* curr_bucket_len = (size_t*)(curr_bucket + info->key_val_pairs_len_offset);
    uint8_t* curr_bucket_key_val_pair_array = *(uint8_t**)(curr_bucket);

    uint8_t* curr_key_val_pair = curr_bucket_key_val_pair_array + ((*curr_bucket_len) * info->key_val_pair_size);
    HashMapString* curr_key = (HashMapString*)(curr_key_val_pair + info->key_offset);

    SDL_memcpy(curr_key_val_pair, element, element_size); // The element offset is 0, so can just write element data directly to start of address
    curr_key->string = MemoryHeap_alloc(memoryHeap, string_key.len);
    curr_key->len = string_key.len;
    SDL_memcpy(curr_key->string, string_key.string, string_key.len * sizeof(char));

    (*curr_bucket_len)++;
    (*(size_t*)((uint8_t*)hashmap + info->hashmap_load_offset))++;

    return true;
}