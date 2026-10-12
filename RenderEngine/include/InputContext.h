#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include "GPUText.h"

#define INPUT_CONTEXT_BASE_BUFFER_SIZE 2000

#define INPUT_CONTEXT_KEY_SEQUENCE_MAX 3

#define INPUT_CONTEXT_TEXT_ARRAY_GROW_AMOUNT 10

typedef struct InputContextLayout {
    float x;
    float y;
    float width;
    float height;
} InputContextLayout;

typedef struct InputContext {
    InputContextLayout layout;
    InputContextLayout prevLayout;
    char* buffer;
    TTF_Font* font;
    GPUText gpuText;
    SDL_Keycode keySequences[INPUT_CONTEXT_KEY_SEQUENCE_MAX];
    int keySequencesLen;
    int bufferLen;
    int bufferCapacity;
    int cursorIndex;
    int cursorX; // Used for rendering the cursor
    int cursorY; // Used for rendering the cursor
    int cursorHeight; // Used for rendering the cursor
    bool bufferWasModified;
    bool cursorWasModified;
    float textColor[4];
    float cursorColor[4];
} InputContext;

// For use with SDL_PollEvent. Also make sure to call SDL_StartTextInput before processing
// the SDL_EVENT_TEXT_ signals.
// The text color can be changed at any point after this by simply setting the textColor field of the struct
// to the desired value. The same goes for the cursorColor.
int InputContextCreate(InputContext* inputContext, AppContext* appContext, TTF_Font* font, float textColor[4], float cursorColor[4], InputContextLayout layout);

// Should be called just before the while (SDL_PollEvent(&event)) loop
static inline int InputContextBegin(InputContext* inputContext) {
    inputContext->bufferWasModified = false;
    inputContext->cursorWasModified = false;
    return 0;
}

// Must be called inside a while (SDL_PollEvent(&event)) event loop. It should not be put inside
// an if statement checking for text input events because it will handle that itself.
int InputContextUpdate(InputContext* inputContext, SDL_Event* event);

int InputContextEnd(InputContext* inputContext);

static inline int InputContextDestroy(InputContext* inputContext) {
    GPUTextDestroy(&inputContext->gpuText);
    SDL_free(inputContext->buffer);
    inputContext->buffer = NULL;
    inputContext->cursorIndex = 0;
    return 0;
}

#endif
