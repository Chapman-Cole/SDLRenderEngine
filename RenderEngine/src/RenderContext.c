#include "RenderContext.h"

// If target is NULL, then the swapchain texture will be rendered to
int RenderContextBegin(RenderContext* renderContext, AppContext* appContext, SDL_GPUTexture* target) {
    if (appContext->time - appContext->lastRenderFrameTime < appContext->latencyEaser * appContext->target_render_time) {
        return false;
    }

    appContext->lastRenderFrameTime = appContext->time;
    renderContext->appContext = appContext;

    // First thing that should be done is flushing the app context so that all
    // pending transfers are pushed to the gpu before rendering in case
    // any of the subsequent calls rely on the pending data
    APP_CONTEXT_FLUSH_TRANSFER_CONTEXT(*renderContext->appContext);

    renderContext->cmdbuf = SDL_AcquireGPUCommandBuffer(appContext->device);
    if (renderContext->cmdbuf == NULL) {
        SDL_Log("Failed to acquire gpu command buffer: %s", SDL_GetError());
        return false;
    }

    if (target == NULL) {
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(renderContext->cmdbuf, appContext->window, &renderContext->texture, &renderContext->renderWidth, &renderContext->renderHeight)) {
            SDL_Log("Failed to acquire swapchain: %s", SDL_GetError());
            SDL_CancelGPUCommandBuffer(renderContext->cmdbuf);
            return false;
        }

        if (renderContext->texture != NULL) {
            SDL_GPUColorTargetInfo colorTargetInfo = {0};
            colorTargetInfo.texture = renderContext->texture;
            colorTargetInfo.clear_color = (SDL_FColor){0.137254901961f, 0.149019607843f, 0.152941176471f, 1.0f};
            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

            renderContext->renderPass = SDL_BeginGPURenderPass(renderContext->cmdbuf, &colorTargetInfo, 1, NULL);
        }

        renderContext->aspectRatio = (float)renderContext->renderHeight / (float)renderContext->renderWidth;
    }

    return true;
}

int RenderContextEnd(RenderContext* renderContext) {
    SDL_EndGPURenderPass(renderContext->renderPass);
    SDL_SubmitGPUCommandBuffer(renderContext->cmdbuf);
    return 0;
}