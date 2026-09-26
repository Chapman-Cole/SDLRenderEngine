#include "FontRegistry.h"

#define BUCKET_GROW_AMOUNT 3

// Use FNV-1a hashing function
Uint32 basic_hash_function(const char* identifier) {
    static const Uint32 FNV_offset_basis = 2166136261;
    static const Uint32 FNV_prime = 16777619;

    Uint32 hash = FNV_offset_basis;

    Uint32 identifierLen = SDL_strlen(identifier);
    for (Uint32 i = 0; i < identifierLen; i++) {
        hash ^= identifier[i];
        hash *= FNV_prime;
    }

    return hash;
}

int FontRegistrySet(FontRegistry* registry, const char* identifier, const char* path, float pts) {
    Uint32 index = basic_hash_function(identifier) % FONT_REGISTRY_HASHMAP_LEN;

    SDL_PropertiesID fontProps = SDL_CreateProperties();
    SDL_SetStringProperty(fontProps, TTF_PROP_FONT_CREATE_FILENAME_STRING, path);
    SDL_SetFloatProperty(fontProps, TTF_PROP_FONT_CREATE_SIZE_FLOAT, pts);
    SDL_SetNumberProperty(fontProps, TTF_PROP_FONT_CREATE_HORIZONTAL_DPI_NUMBER, 96 * registry->appContext->displayScale);
    SDL_SetNumberProperty(fontProps, TTF_PROP_FONT_CREATE_VERTICAL_DPI_NUMBER, 96 * registry->appContext->displayScale);

    TTF_Font* font = TTF_OpenFontWithProperties(fontProps);
    if (font == NULL) {
        SDL_Log("Font registry failed to open font %s: %s", path, SDL_GetError());
    }
    SDL_DestroyProperties(fontProps);

    Uint32 entryIdentifierLen = SDL_strlen(identifier);
    char* entryIdentifier = SDL_malloc(entryIdentifierLen * sizeof(char));
    if (entryIdentifier == NULL) {
        TTF_CloseFont(font);
        SDL_Log("Failed to allocate memory for entryIdentifier: %s", SDL_GetError());
        return -1;
    }

    if (registry->hashmap[index].len == 0) {
        registry->hashmap[index].capacity = BUCKET_GROW_AMOUNT;
        registry->hashmap[index].arr = SDL_malloc(registry->hashmap[index].capacity * sizeof(FontRegistryEntry));
        if (registry->hashmap[index].arr == NULL) {
            TTF_CloseFont(font);
            SDL_free(entryIdentifier);
            SDL_Log("Failed to allocate memory for a font registry bucket: %s", SDL_GetError());
            return -1;
        }

        registry->hashmap[index].arr[registry->hashmap[index].len] = (FontRegistryEntry){
            .font = font,
            .identifier = entryIdentifier};

        registry->hashmap[index].len++;
    } else {
        // Look to see if the key has already been added
        for (Uint32 i = 0; i < registry->hashmap[index].len; i++) {
            if (SDL_strcmp(identifier, registry->hashmap[index].arr[i].identifier) == 0) {
                TTF_CloseFont(registry->hashmap[index].arr[i].font);
                SDL_free(registry->hashmap[index].arr[i].identifier);
                registry->hashmap[index].arr[i] = (FontRegistryEntry){
                    .font = font,
                    .identifier = entryIdentifier,
                };

                return 0;
            }
        }

        if (registry->hashmap[index].len + 1 >= registry->hashmap[index].capacity) {
            registry->hashmap[index].capacity += BUCKET_GROW_AMOUNT;
            void* temp_ptr = SDL_realloc(registry->hashmap[index].arr, registry->hashmap[index].capacity * sizeof(FontRegistryEntry));
            if (temp_ptr == NULL) {
                TTF_CloseFont(font);
                SDL_free(entryIdentifier);
                SDL_Log("Failed to re-allocate memory when adding to FontRegistry: %s", SDL_GetError());
                return -1;
            }
            registry->hashmap[index].arr = (FontRegistryEntry*)temp_ptr;
        }

        registry->hashmap[index].arr[registry->hashmap[index].len] = (FontRegistryEntry){
            .font = font,
            .identifier = entryIdentifier,
        };

        registry->hashmap[index].len++;
    }

    return 0;
}

TTF_Font* FontRegistryGet(FontRegistry* registry, const char* identifier) {
    Uint32 index = basic_hash_function(identifier) % FONT_REGISTRY_HASHMAP_LEN;

    if (registry->hashmap[index].len == 1) {
        return registry->hashmap[index].arr[0].font;
    } else {
        for (Uint32 i = 0; i < registry->hashmap[index].len; i++) {
            if (SDL_strcmp(identifier, registry->hashmap[index].arr[i].identifier) == 0) {
                return registry->hashmap[index].arr[i].font;
            }
        }
    }

    return NULL;
}

int FontRegistryRemove(FontRegistry* registry, const char* identifier) {
    Uint32 index = basic_hash_function(identifier) % FONT_REGISTRY_HASHMAP_LEN;

    if (registry->hashmap[index].len == 1) {
        TTF_CloseFont(registry->hashmap[index].arr[0].font);
        SDL_free(registry->hashmap[index].arr[0].identifier);

        registry->hashmap[index].len--;
        SDL_free(registry->hashmap[index].arr);
        registry->hashmap[index].capacity = 0;
        return 0;
    } else {
        int64_t removeIndex = -1;
        for (Uint32 i = 0; i < registry->hashmap[index].len; i++) {
            if (SDL_strcmp(identifier, registry->hashmap[index].arr[i].identifier) == 0) {
                TTF_CloseFont(registry->hashmap[index].arr[i].font);
                SDL_free(registry->hashmap[index].arr[i].identifier);
                removeIndex = i;
                break;
            }
        }

        if (removeIndex > 0 && removeIndex == registry->hashmap[index].len - 1) {
            registry->hashmap[index].len--;

            if (registry->hashmap[index].len < registry->hashmap[index].capacity - BUCKET_GROW_AMOUNT) {
                registry->hashmap[index].capacity -= BUCKET_GROW_AMOUNT;
                void* temp_ptr = SDL_realloc(registry->hashmap[index].arr, registry->hashmap[index].capacity * sizeof(FontRegistryEntry));
                if (temp_ptr == NULL) {
                    SDL_Log("Failed to re-allocate memory when removing from FontRegistry: %s", SDL_GetError());
                    return -1;
                }
                registry->hashmap[index].arr = (FontRegistryEntry*)temp_ptr;
            }
            return 0;
        } else if (removeIndex >= 0) {
            SDL_memmove(registry->hashmap[index].arr + removeIndex, registry->hashmap[index].arr + removeIndex + 1, (registry->hashmap[index].len - removeIndex - 1) * sizeof(FontRegistryEntry));

            registry->hashmap[index].len--;

            if (registry->hashmap[index].len < registry->hashmap[index].capacity - BUCKET_GROW_AMOUNT) {
                registry->hashmap[index].capacity -= BUCKET_GROW_AMOUNT;
                void* temp_ptr = SDL_realloc(registry->hashmap[index].arr, registry->hashmap[index].capacity * sizeof(FontRegistryEntry));
                if (temp_ptr == NULL) {
                    SDL_Log("Failed to re-allocate memory when removing from FontRegistry: %s", SDL_GetError());
                    return -1;
                }
                registry->hashmap[index].arr = (FontRegistryEntry*)temp_ptr;
            }
            return 0;
        }
    }

    return -1;
}

int FontRegistryDestroy(FontRegistry* registry) {
    for (Uint32 i = 0; i < FONT_REGISTRY_HASHMAP_LEN; i++) {
        if (registry->hashmap[i].len > 0) {
            for (Uint32 j = 0; j < registry->hashmap[i].len; j++) {
                TTF_CloseFont(registry->hashmap[i].arr[j].font);
                SDL_free(registry->hashmap[i].arr[j].identifier);
            }
            SDL_free(registry->hashmap[i].arr);
        }
    }
    return 0;
}