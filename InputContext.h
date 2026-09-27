#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include "GPUText.h"
#include "SDL3/SDL.h"

#define INPUT_CONTEXT_BASE_BUFFER_SIZE 2000

#define INPUT_CONTEXT_KEY_SEQUENCE_MAX 3

typedef struct InputContext {
    char* buffer;
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
} InputContext;

// For use with SDL_PollEvent. Also make sure to call SDL_StartTextInput before processing
// the SDL_EVENT_TEXT_ signals.
static inline int InputContextCreate(InputContext* inputContext, AppContext* appContext, TTF_Font* font) {
    inputContext->bufferCapacity = INPUT_CONTEXT_BASE_BUFFER_SIZE;
    inputContext->bufferLen = 0;

    inputContext->buffer = SDL_malloc(inputContext->bufferCapacity * sizeof(char));
    if (inputContext->buffer == NULL) {
        SDL_Log("Failed to allocate memory for InputContext: %s", SDL_GetError());
        return -1;
    }

    // Make sure the buffer starts out as null terminated
    inputContext->buffer[0] = '\0';
    inputContext->cursorIndex = 0;

    inputContext->cursorHeight = TTF_GetFontHeight(font);
    inputContext->cursorX = 0;
    // Initialize to cursor negative height since text is drawn from bottom left up
    inputContext->cursorY = -inputContext->cursorHeight;

    inputContext->keySequencesLen = 0;
    SDL_zeroa(inputContext->keySequences);

    GPUTextCreate(&inputContext->gpuText, appContext, font, inputContext->buffer);
    return 0;
}

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