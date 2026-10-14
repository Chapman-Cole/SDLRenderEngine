#ifndef CLAYCOMPOSITOR_H
#define CLAYCOMPOSITOR_H

#include "FontRegistry.h"
#include "GPUText.h"
#include "RenderContext.h"
#include "clay.h"
#include "DynamicArray.h"

#define CLAY_COMPOSITOR_TEXT_OBJECT_ARRAY_GROW_AMOUNT 20

typedef struct GPUTextInfo {
    GPUText text;
    Uint32 textLen;
    bool taggedForDeletion;
} GPUTextInfo;

typedef struct GPUTextInfoArray {
    GPUTextInfo* arr;
    size_t len;
    size_t capacity;
} GPUTextInfoArray;

typedef struct ClayCompositor {
    GPUTextInfoArray textObjectCache;
    FontRegistry* fontRegistry;
    AppContext* appContext;
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
    clayCompositor->fontRegistry = fontRegistry;
    DYNAMIC_ARRAY_INIT(clayCompositor->textObjectCache);
    return 0;
}

void ClayCompositorRenderCommands(ClayCompositor* clayCompositor, RenderContext* renderContext, Clay_RenderCommandArray* renderCommands);

static inline void ClayCompositorDestroy(ClayCompositor* clayCompositor) {
    for (uint32_t i = 0; i < clayCompositor->textObjectCache.len; i++) {
        GPUTextDestroy(&clayCompositor->textObjectCache.arr[i].text);
    }
    DYNAMIC_ARRAY_FREE(clayCompositor->textObjectCache);
}

#endif
