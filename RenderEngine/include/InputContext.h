#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include "SDL3_ttf/SDL_ttf.h"
#include "AppContext.h"

typedef struct InputContextLayout {
    float x;
    float y;
    float width;
    float height;
} InputContextLayout;

typedef struct TextLine {
    char* arr;
    size_t len;
    size_t capacity;
} TextLine;

typedef struct TextLineArray {
    TextLine* arr;
    size_t len;
    size_t capacity;
} TextLineArray;

typedef struct TextRenderInfo {
    TTF_Text* text;
} TextRenderInfo;

typedef struct TextRenderInfoArray {
    TextRenderInfo* arr;
    size_t len;
    size_t capacity;
} TextRenderInfoArray;

typedef struct InputContext {
    InputContextLayout layout;
    InputContextLayout prevLayout;
    TextLineArray buffer;
    TextRenderInfoArray renderInfo;
    TTF_Font* font;
    GPUBuffer* vertexBuffer;
    GPUBuffer* indexBuffer;
    int cursorLine;
    int cursorIndex;
    int cursorX; // Used for rendering the cursor
    int cursorY; // Used for rendering the cursor
    int cursorHeight; // Used for rendering the cursor
    bool bufferWasModified;
    bool cursorWasModified;
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

#endif
