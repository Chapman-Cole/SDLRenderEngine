#include "RenderContext.h"

// If target is NULL, then the swapchain texture will be rendered to
int RenderContextBegin(RenderContext* renderContext, AppContext* appContext, SDL_GPUTexture* target) {
    renderContext->appContext = appContext;

    renderContext->cmdbuf = SDL_AcquireGPUCommandBuffer(appContext->device);
    if (renderContext->cmdbuf == NULL) {
        SDL_Log("Failed to acquire gpu command buffer: %s", SDL_GetError());
        return -1;
    }

    Uint32 renderWidth, renderHeight;
    if (target == NULL) {
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(renderContext->cmdbuf, appContext->window, &renderContext->texture, &renderWidth, &renderHeight)) {
            SDL_Log("Failed to acquire swapchain.\n");
            SDL_SubmitGPUCommandBuffer(renderContext->cmdbuf);
            return -1;
        }

        if (renderContext->texture != NULL) {
            SDL_GPUColorTargetInfo colorTargetInfo = {0};
            colorTargetInfo.texture = renderContext->texture;
            colorTargetInfo.clear_color = (SDL_FColor){0.137254901961f, 0.149019607843f, 0.152941176471f, 1.0f};
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            renderContext->renderPass = SDL_BeginGPURenderPass(renderContext->cmdbuf, &colorTargetInfo, 1, NULL);
        }

        renderContext->renderWidth = (float)renderWidth;
        renderContext->renderHeight = (float)renderHeight;
        renderContext->aspectRatio = renderContext->renderHeight / renderContext->renderWidth;
    }

    return 0;
}

int RenderContextEnd(RenderContext* renderContext) {
    SDL_EndGPURenderPass(renderContext->renderPass);
    SDL_SubmitGPUCommandBuffer(renderContext->cmdbuf);
    return 0;
}