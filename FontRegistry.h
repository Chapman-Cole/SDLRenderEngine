#ifndef FONTREGISTRY_H
#define FONTREGISTRY_H

#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "AppContext.h"

// May want to increase this later if it looks like a lot of collisions occur
#define FONT_REGISTRY_HASHMAP_LEN 101

typedef struct FontRegistryEntry {
    TTF_Font* font;
    char* identifier;
} FontRegistryEntry;

typedef struct FontRegistryArray {
    FontRegistryEntry* arr;
    Uint32 len;
    Uint32 capacity;
} FontRegistryArray;

// The registry will be implemented as a hashmap
typedef struct FontRegistry {
    FontRegistryArray hashmap[FONT_REGISTRY_HASHMAP_LEN];
    AppContext* appContext;
} FontRegistry;

static inline int FontRegistryCreate(FontRegistry* registry, AppContext* appContext) {
    registry->appContext = appContext;
    SDL_zero(registry->hashmap);
    return 0;
}

// A good naming pattern for the identifier of the fonts is the name of the font with the pt size appended at the end
int FontRegistrySet(FontRegistry* registry, const char* identifier, const char* path, float pts);

TTF_Font* FontRegistryGet(FontRegistry* registry, const char* identifier);

int FontRegistryRemove(FontRegistry* registry, const char* identifier);

int FontRegistryDestroy(FontRegistry* registry);

#endif