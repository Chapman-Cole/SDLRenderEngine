#ifndef GPUTEXT_H
#define GPUTEXT_H

#include "SDL3_ttf/SDL_ttf.h"
#include "AppContext.h"
#include "SDL3/SDL.h"

typedef struct TextSequence {
    SDL_GPUTexture* atlas;
    SDL_GPUBuffer* vertexBuffer;
    SDL_GPUBuffer* indexBuffer;
    Uint32 numIndices;
} TextSequence;

typedef struct GPUText {
    TTF_Text* text;
    TTF_Font* font;
    AppContext* appContext;
    TextSequence* atlasses;
    Uint32 numAtlasses;
    int width;
    int height;
    int x;
    int y;
} GPUText;

// A note about fonts: When rendering fonts on high dpi screens, you will want to account for the display scale
// factor when setting the font size. The best way to do this is with TTF_SetFontSizeDPI, which would look something
// like: TTF_SetFontSizeDPI(font, 15.0, 96.0 * appContext.displayScale, 96.0 * appContext.displayScale);
// the 96 is important here because the base dpi used by most programs when the display scale is 1.0
// is 96 ppi
// Also, you must provide a stable reference to a font, because if the font size changes it has to 
// be able to re-generate the text.
int GPUTextCreate(GPUText* renderText, AppContext* appContext, TTF_Font* font, const char* text);

int GPUTextDestroy(GPUText* renderText);

int GPUTextUpdate(GPUText* renderText, const char* text);

#endif