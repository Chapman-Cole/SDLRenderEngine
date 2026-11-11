#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "GPUTransferContext.h"
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"

#define APP_CONTEXT_DEVICE_DRIVER_VULKAN "vulkan"
#define APP_CONTEXT_DEVICE_DRIVER_DIRECT3D12 "direct3d12"
#define APP_CONTEXT_DEVICE_DRIVER_AGNOSTIC NULL

#define APP_CONTEXT_BUILTIN_PIPELINES_MAX 12

enum BuiltinPipelineIndex {
    BUILTIN_PIPELINE_RECTANGLE,
    BUILTIN_PIPELINE_CIRCLE,
    BUILTIN_PIPELINE_TEXT,
    BUILTIN_PIPELINE_TEXT_CURSOR,
};

typedef struct AppContext {
    GPUTransferContext transferContext;
    const char* deviceName;
    const char* windowName;
    SDL_Window* window;
    SDL_GPUDevice* device;
    TTF_TextEngine* textEngine;
    SDL_GPUGraphicsPipeline* builtinPipelines[APP_CONTEXT_BUILTIN_PIPELINES_MAX];
    SDL_GPUSampler* textSampler;
    SDL_GPUShaderFormat shaderFormat;
    float displayScale;
    double time;
    double deltaTime;
    float mouseLogicalX;
    float mouseLogicalY;
    float mousePixelX;
    float mousePixelY;
    bool isLeftMouseDown;
} AppContext;

#define APP_CONTEXT_DEVICE(appContext) ((appContext).device)

// Forces the app context to immediately submit all pending gpu uploads
#define APP_CONTEXT_FLUSH_TRANSFER_CONTEXT(appContext) (GPUTransferContextSubmit(&((appContext).transferContext)));

int AppContextInit(AppContext* app_context, SDL_WindowFlags windowFlags, bool gpuDebugEnabled, size_t transferContextArenaSize);

// Must be called at the start of every frame to get accurate updates of the current time value and the delta time
static inline void AppContextUpdate(AppContext* app_context) {
    double prevTime = app_context->time;
    app_context->time = (double)SDL_GetTicksNS() / 1'000'000'000.0;
    app_context->deltaTime = app_context->time - prevTime;

    SDL_GetMouseState(&app_context->mouseLogicalX, &app_context->mouseLogicalY);

    int logicalWindowSizeX, logicalWindowSizeY;
    SDL_GetWindowSize(app_context->window, &logicalWindowSizeX, &logicalWindowSizeY);

    int pixelWindowSizeX, pixelWindowSizeY;
    SDL_GetWindowSizeInPixels(app_context->window, &pixelWindowSizeX, &pixelWindowSizeY);

    float mouseScaleX = (float)pixelWindowSizeX / (float)logicalWindowSizeX;
    float mouseScaleY = (float)pixelWindowSizeY / (float)logicalWindowSizeY;

    app_context->mousePixelX = app_context->mouseLogicalX * mouseScaleX;
    app_context->mousePixelY = app_context->mouseLogicalY * mouseScaleY;
}

static inline void AppContextProcessEvent(AppContext* app_context, SDL_Event* event) {
    switch (event->type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                app_context->isLeftMouseDown = true;
            }
        } break;

        case SDL_EVENT_MOUSE_BUTTON_UP: {
            if (event->button.button == SDL_BUTTON_LEFT) {
                app_context->isLeftMouseDown = false;
            }
        } break;
    }
}

int AppContextTerminate(AppContext* app_context);

#endif
