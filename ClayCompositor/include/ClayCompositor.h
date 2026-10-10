#ifndef CLAYCOMPOSITOR_H
#define CLAYCOMPOSITOR_H

#include "FontRegistry.h"
#include "GPUText.h"
#include "RenderContext.h"
#include "clay.h"

#define CLAY_COMPOSITOR_TEXT_OBJECT_ARRAY_GROW_AMOUNT 20

typedef struct ClayCompositor {
    FontRegistry* fontRegistry;
    AppContext* appContext;
    GPUText* textObjects;
    uint32_t textObjectsLen;
    uint32_t textObjectsCapacity;
} ClayCompositor;

typedef enum { COMPOSITOR_EXTENSION_TYPE_INPUT_CONTEXT } CompositorExtensionType;

// The initialization of a CLAY element using a compositor extension would look
// like CLAY(CLAY_ID("CompositorExtensionID"), { .custom = { .customData =
// CompositorExtensionData }})
typedef struct CompositorExtensionData {
    CompositorExtensionType type;
    union {
        InputContext* inputContext;
    };
} CompositorExtensionData;

static inline int ClayCompositorCreate(ClayCompositor* clayCompositor, AppContext* appContext, FontRegistry* fontRegistry) {
    clayCompositor->appContext = appContext;
    clayCompositor->textObjectsLen = 0;
    clayCompositor->textObjectsCapacity = CLAY_COMPOSITOR_TEXT_OBJECT_ARRAY_GROW_AMOUNT;
    clayCompositor->textObjects = SDL_malloc(clayCompositor->textObjectsCapacity * sizeof(GPUText));
    if (clayCompositor->textObjects == NULL) {
        SDL_Log("Failed to allocate memory for text objects in clay compositor: %s", SDL_GetError());
        return -1;
    }
    clayCompositor->fontRegistry = fontRegistry;
    return 0;
}

void ClayCompositorRenderCommands(ClayCompositor* clayCompositor, RenderContext* renderContext, Clay_RenderCommandArray* renderCommands);

static inline void ClayCompositorDestroy(ClayCompositor* clayCompositor) {
    for (uint32_t i = 0; i < clayCompositor->textObjectsLen; i++) {
        GPUTextDestroy(&clayCompositor->textObjects[i]);
    }
    clayCompositor->textObjectsLen = 0;
    SDL_free(clayCompositor->textObjects);
}

#endif
