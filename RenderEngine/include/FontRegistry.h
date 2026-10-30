#ifndef FONTREGISTRY_H
#define FONTREGISTRY_H

#include "AppContext.h"

// Important to keep this value because powers of two allow for fast indexing
// by index = hash & (capacity - 1)
// Must also be this size in order to be compatible with the size of the FontRegistryKey
#define FONT_REGISTRY_HASHMAP_LEN 256

typedef struct FontRegistryEntry {
    TTF_Font* font;
    char* identifier;
} FontRegistryEntry;

typedef struct FontRegistryArray {
    FontRegistryEntry* arr;
    Uint8 len;
    Uint8 capacity;
} FontRegistryArray;

// The registry will be implemented as a hashmap
typedef struct FontRegistry {
    FontRegistryArray hashmap[FONT_REGISTRY_HASHMAP_LEN];
    AppContext* appContext;
} FontRegistry;

// the lower 8 bits are an index into the hashmap array and the higher 8 bits are an index into
// into that particular bucket
typedef uint16_t FontRegistryKey;

static inline int FontRegistryCreate(FontRegistry* registry, AppContext* appContext) {
    registry->appContext = appContext;
    SDL_zero(registry->hashmap);

    return 0;
}

// A good naming pattern for the identifier of the fonts is the name of the font with the pt size appended at the end
int FontRegistrySet(FontRegistry* registry, const char* identifier, const char* path, float pts);

TTF_Font* FontRegistryGet(FontRegistry* registry, const char* identifier);

FontRegistryKey FontRegistryGenKey(FontRegistry* registry, const char* identifier);

TTF_Font* FontRegistryGetWithKey(FontRegistry* registry, FontRegistryKey key);

int FontRegistryRemove(FontRegistry* registry, const char* identifier);

int FontRegistryDestroy(FontRegistry* registry);

#endif
