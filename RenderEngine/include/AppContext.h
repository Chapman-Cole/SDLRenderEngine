#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "GPUTransferContext.h"
#include "PerformanceQuery.h"
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "limits.h"

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

#define APP_CONTEXT_TARGET_FPS_UNLIMITED INT_MAX

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
    double lastRenderFrameTime;
    float target_render_time;
    bool vsyncEnabled;
    float refreshRate;
    float latencyEaser;
    int latencyFrameReductionCount;
} AppContext;

#define APP_CONTEXT_DEVICE(appContext) ((appContext).device)

// Forces the app context to immediately submit all pending gpu uploads
#define APP_CONTEXT_FLUSH_TRANSFER_CONTEXT(appContext) (GPUTransferContextSubmit(&((appContext).transferContext)));

static inline float __app_context_calculate_latency_easer(AppContext* app_context, float num_frames) {
    // Basic idea here is to calculate the percentage of the new target fps compared to the old
    // target fps (assuming the target fps is the refreshRate), which would be (refreshRate - numFrames) / refreshRate
    // However, since the latencyEaser is multiplied by the target_render_time, this has to be
    // 1 over the previous value, giving:
    return (float)(app_context->refreshRate) / (float)(app_context->refreshRate - num_frames);
}

// A latency easing of 1-3 frames (generally higher for a larger refresh rate) should help
// quite a bit with input latency when vsync is enabled.
static inline void AppContextSetLatencyEasing(AppContext* app_context, float num_frames) {
    app_context->latencyFrameReductionCount = num_frames;
    app_context->latencyEaser = __app_context_calculate_latency_easer(app_context, num_frames);
}

int AppContextInit(AppContext* app_context, SDL_WindowFlags windowFlags, bool gpuDebugEnabled, size_t transferContextArenaSize);

// Must be called at the start of every frame to get accurate updates of the current time value and the delta time
static inline void AppContextUpdate(AppContext* app_context) {
    app_context->time = (double)SDL_GetTicksNS() / 1'000'000'000.0;
    app_context->deltaTime = app_context->time - app_context->lastRenderFrameTime;

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

    case SDL_EVENT_WINDOW_DISPLAY_CHANGED: {
        SDL_DisplayID displayId = SDL_GetDisplayForWindow(app_context->window);
        const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(displayId);
        app_context->refreshRate = displayMode->refresh_rate;
        if (app_context->vsyncEnabled == true) {
            app_context->target_render_time = 1.0f / displayMode->refresh_rate;
            app_context->latencyEaser = __app_context_calculate_latency_easer(app_context, app_context->latencyFrameReductionCount);
        }
    } break;

    case SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED: {
        SDL_DisplayID displayId = SDL_GetDisplayForWindow(app_context->window);
        const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(displayId);
        app_context->refreshRate = displayMode->refresh_rate;
        if (app_context->vsyncEnabled == true) {
            app_context->target_render_time = 1.0f / displayMode->refresh_rate;
            app_context->latencyEaser = __app_context_calculate_latency_easer(app_context, app_context->latencyFrameReductionCount);
        }
    } break;

    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
        app_context->displayScale = SDL_GetWindowDisplayScale(app_context->window);
    } break;
    }
}

int AppContextTerminate(AppContext* app_context);

static inline void AppContextSetTargetFps(AppContext* app_context, float target_fps) {
    if (app_context->vsyncEnabled == false) {
        app_context->target_render_time = 1.0f / target_fps;
    }
}

static inline void AppContextSetSwapchainParameters(AppContext* app_context, SDL_GPUSwapchainComposition swapchain_composition, SDL_GPUPresentMode present_mode) {
    if (
        SDL_WindowSupportsGPUPresentMode(app_context->device, app_context->window, present_mode) == false ||
        SDL_WindowSupportsGPUSwapchainComposition(app_context->device, app_context->window, swapchain_composition) == false) {
        SDL_Log("Unsupported swapchain composition or present_mode.\n");
        return;
    }

    SDL_SetGPUSwapchainParameters(app_context->device, app_context->window, swapchain_composition, present_mode);
    if (present_mode == SDL_GPU_PRESENTMODE_VSYNC) {
        app_context->vsyncEnabled = true;
        SDL_DisplayID displayId = SDL_GetDisplayForWindow(app_context->window);
        const SDL_DisplayMode* displayMode = SDL_GetDesktopDisplayMode(displayId);
        app_context->target_render_time = 1.0f / displayMode->refresh_rate;
    } else {
        app_context->vsyncEnabled = false;
    }
}

#endif
