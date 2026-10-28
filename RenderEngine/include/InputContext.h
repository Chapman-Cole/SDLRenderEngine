#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include "AppContext.h"
#include "SDL3_ttf/SDL_ttf.h"

typedef struct TextRenderInfoSequence {
    SDL_GPUTexture* atlas;
    GPUBuffer* vertexBuffer;
    GPUBuffer* indexBuffer;
    Uint32 numIndices;
} TextRenderInfoSequence;

typedef struct TextRenderInfoSequenceArray {
    TextRenderInfoSequence* arr;
    size_t len;
    size_t capacity;
} TextRenderInfoSequenceArray;

typedef struct TextRenderInfo {
    TTF_Text* text;
    TextRenderInfoSequenceArray sequence;
    int textWidth;
    int textHeight;
} TextRenderInfo;

typedef struct InputContextLayout {
    float x;
    float y;
    float width;
    float height;
} InputContextLayout;

typedef struct TextLine {
    TextRenderInfo renderInfo;
    char* arr;
    size_t len;
    size_t capacity;
    bool lineWasUpdated;
} TextLine;

typedef struct TextLineArray {
    TextLine* arr;
    size_t len;
    size_t capacity;
} TextLineArray;

#define INPUT_CONTEXT_BUFFER_WAS_MODIFIED (1 << 0)
#define INPUT_CONTEXT_CURSOR_WAS_MODIFIED (1 << 1)
#define INPUT_CONTEXT_LAYOUT_WAS_MODIFIED (1 << 2)
#define INPUT_CONTEXT_FONT_WAS_CHANGED (1 << 3)

typedef struct InputContext {
    InputContextLayout layout;
    TextLineArray buffer;
    TTF_Font* font;
    AppContext* app_context;
    int cursorLine;
    int cursorIndex;
    int cursorX;      // Used for rendering the cursor
    int cursorY;      // Used for rendering the cursor
    int cursorHeight; // Used for rendering the cursor
    int line_spacing;
    uint32_t flags;
    float textColor[4];
    float cursorColor[4];
} InputContext;

// For use with SDL_PollEvent (or inside SDL_AppEvent callback). Also make sure to call
// SDL_StartTextInput before processing the SDL_EVENT_TEXT_ signals.
// The text color can be changed at any point after this by simply setting the textColor field of the struct
// to the desired value. The same goes for the cursorColor.
int InputContextCreate(InputContext* inputContext, AppContext* appContext, TTF_Font* font, float textColor[4], float cursorColor[4], InputContextLayout layout);

// Must be called inside a while (SDL_PollEvent(&event)) event loop. It should not be put inside
// an if statement checking for text input events because it will handle that itself.
int InputContextProcessEvent(InputContext* inputContext, SDL_Event* event);

// Should be called inside SDL_AppIterate callback or placed inside main loop
int InputContextRefresh(InputContext* inputContext);

int InputContextDestroy(InputContext* inputContext);

static inline void InputContextUpdateLayout(InputContext* inputContext, InputContextLayout layout) {
    inputContext->layout = layout;
    inputContext->flags |= INPUT_CONTEXT_LAYOUT_WAS_MODIFIED;
}

static inline void InputContextSetFont(InputContext* inputContext, TTF_Font* font) {
    inputContext->font = font;
    inputContext->cursorHeight = TTF_GetFontHeight(font);
    inputContext->line_spacing = inputContext->cursorHeight;
    inputContext->flags |= INPUT_CONTEXT_FONT_WAS_CHANGED;
}

#endif