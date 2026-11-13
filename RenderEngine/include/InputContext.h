#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include "AppContext.h"
#include "SDL3_ttf/SDL_ttf.h"

typedef enum TextRenderInfoImageType {
    TEXT_RENDER_INFO_IMAGE_TYPE_ALPHA_ONLY,
    TEXT_RENDER_INFO_IMAGE_TYPE_COLOR,
} TextRenderInfoImageType;

typedef struct TextRenderInfoSequence {
    SDL_GPUTexture* atlas;
    GPUBuffer* vertexBuffer;
    GPUBuffer* indexBuffer;
    Uint32 numIndices;
    TextRenderInfoImageType imageType;
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

typedef struct TextRenderInfoArray {
    TextRenderInfo* arr;
    size_t len;
    size_t capacity;
} TextRenderInfoArray;

typedef struct InputContextLayout {
    int x;
    int y;
    int width;
    int height;
} InputContextLayout;

typedef enum TextCursorWrapState {
    TEXT_CURSOR_WRAP_STATE_UP,
    TEXT_CURSOR_WRAP_STATE_DOWN,
} TextCursorWrapState;

typedef struct TextLine {
    TextRenderInfoArray renderInfo;
    char* arr;
    size_t len;
    size_t capacity;
    int criticalIndex;
    TextCursorWrapState wrapState;
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

typedef enum InputContextLayoutMode {
    // Will ignore the width and height of the provided layout struct (offsets will still be used though)
    INPUT_CONTEXT_LAYOUT_MODE_UNBOUNDED,

    // only the horiztonal portion of the string that fits will be rendered
    // only the lines that fit vertically in the bounds will be rendered
    INPUT_CONTEXT_LAYOUT_MODE_BOUNDED_CUTOFF,

    // Text will be cutoff, but the scroll offsets can be updated to get an offset view into the text
    // buffer
    INPUT_CONTEXT_LAYOUT_MODE_SCROLLABLE,

    // Text will be wrapped according to the horizontal bounds of the layout
    INPUT_CONTEXT_LAYOUT_MODE_WRAP,
} InputContextLayoutMode;

typedef struct InputContextIndexArray {
    int* arr;
    size_t len;
    size_t capacity;
} InputContextIndexArray;

typedef struct InputContext {
    InputContextLayout layout;
    TextLineArray buffer;
    InputContextIndexArray modifiedLines;
    TTF_Font* font;
    AppContext* app_context;
    int cursorLine;
    int cursorIndex;
    int cursorX;
    int cursorY;
    int cursorHeight;
    int line_spacing;
    uint32_t flags;
    float textColor[4];
    float cursorColor[4];
    float highlightColor[4];
    InputContextLayoutMode layoutMode;
    int tabWidth;
    int maxLineWidth;
    int maxHeight;
    float scrollMaxHorizontalOffset;
    float scrollHorizontalOffset;
    float scrollMaxVerticalOffset;
    float scrollVerticalOffset;
    int lineRenderIndexStart;
    int lineRenderIndexEnd;
    int lineRenderOffsetY;
    bool mouseLeftDown;
    int highlightStartIndex;
    int highlightStartLineIndex;
    float cursorBlinkDelay;
    float cursorBlinkTimer;
    bool cursorWasInBounds;
    SDL_Cursor* textCursor;
} InputContext;

// For use with SDL_PollEvent (or inside SDL_AppEvent callback). Also make sure to call
// SDL_StartTextInput before processing the SDL_EVENT_TEXT signals.
// The text color can be changed at any point after this by simply setting the textColor field of the struct
// to the desired value. The same goes for the cursorColor.
// defaults to INPUT_CONTEXT_LAYOUT_MODE_UNBOUNDED, but that can easily be updated with its corresponding function
int InputContextCreate(InputContext* inputContext, AppContext* appContext, TTF_Font* font, float textColor[4], float cursorColor[4], InputContextLayout layout);

// Must be called inside a while (SDL_PollEvent(&event)) event loop. It should not be put inside
// an if statement checking for text input events because it will handle that itself.
int InputContextProcessEvent(InputContext* inputContext, SDL_Event* event);

// Should be called inside SDL_AppIterate callback or placed inside main loop
int InputContextRefresh(InputContext* inputContext);

int InputContextDestroy(InputContext* inputContext);

int InputContextClear(InputContext* inputContext);

int InputContextOpenFile(InputContext* inputContext, const char* file_path);

static inline void InputContextUpdateLayout(InputContext* inputContext, InputContextLayout layout) {
    if (
        inputContext->layout.x != layout.x ||
        inputContext->layout.y != layout.y ||
        inputContext->layout.width != layout.width ||
        inputContext->layout.height != layout.height) {
        inputContext->flags |= INPUT_CONTEXT_LAYOUT_WAS_MODIFIED;
        inputContext->layout = layout;
    }
}

static inline void InputContextSetFont(InputContext* inputContext, TTF_Font* font) {
    if (font != NULL) {
        inputContext->font = font;
        inputContext->cursorHeight = TTF_GetFontHeight(font);
        inputContext->line_spacing = TTF_GetFontLineSkip(font);
        inputContext->flags |= INPUT_CONTEXT_FONT_WAS_CHANGED;
    }
}

static inline void InputContextSetTextColor(InputContext* inputContext, float color[4]) {
    inputContext->textColor[0] = color[0];
    inputContext->textColor[1] = color[1];
    inputContext->textColor[2] = color[2];
    inputContext->textColor[3] = color[3];
}

static inline void InputContextSetTextCursorColor(InputContext* inputContext, float color[4]) {
    inputContext->cursorColor[0] = color[0];
    inputContext->cursorColor[1] = color[1];
    inputContext->cursorColor[2] = color[2];
    inputContext->cursorColor[3] = color[3];
}

static inline void InputContextSetLayoutMode(InputContext* inputContext, InputContextLayoutMode mode) {
    inputContext->layoutMode = mode;
}

void InputContextSetHorizontalScrollOffset(InputContext* inputContext, int h_scroll_amount);

void InputContextSetVerticalScrollOffset(InputContext* inputContext, int v_scroll_amount);

static inline void InputContextSetTabWidth(InputContext* inputContext, int tabWidth) {
    inputContext->tabWidth = tabWidth;
}

static inline void InputContextSetHighlightColor(InputContext* inputContext, float color[4]) {
    inputContext->highlightColor[0] = color[0];
    inputContext->highlightColor[1] = color[1];
    inputContext->highlightColor[2] = color[2];
    inputContext->highlightColor[3] = color[3];
}

static inline void InputContextSetBlinkDelay(InputContext* inputContext, float blink_delay) {
    inputContext->cursorBlinkDelay = blink_delay;
}

#endif
