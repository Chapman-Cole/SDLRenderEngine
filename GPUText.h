#ifndef GPUTEXT_H
#define GPUTEXT_H

#include "SDL3_ttf/SDL_ttf.h"
#include "AppContext.h"
#include "SDL3/SDL.h"

typedef struct GPUText {
    TTF_Text* text;
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    AppContext* appContext;
    Uint32 numIndices;
} GPUText;

int GPUTextCreate(GPUText* renderText, AppContext* appContext, TTF_Font* font, const char* text);

int GPUTextDestroy(GPUText* renderText);

#endif