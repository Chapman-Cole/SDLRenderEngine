#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include "SDL3/SDL.h"
#include "AppContext.h"

typedef struct RenderContext {
    AppContext* appContext;
    SDL_GPUCommandBuffer* cmdbuf;
    SDL_GPUTexture* texture; // will typically be the swapchain texture
    SDL_GPURenderPass* renderPass;
    float renderWidth;
    float renderHeight;
    float aspectRatio;
} RenderContext;

// If target is NULL, then the swapchain texture will be rendered to
int RenderContextBegin(RenderContext* renderContext, AppContext* appContext, SDL_GPUTexture* target);

int RenderContextEnd(RenderContext* renderContext);

#endif