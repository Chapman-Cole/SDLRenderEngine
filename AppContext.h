#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "SDL3/SDL.h"

#define APP_CONTEXT_BUILTIN_PIPELINES_MAX 12

enum BuiltinPipelineIndex {
    BUILTIN_PIPELINE_RECTANGLE,
    BUILTIN_PIPELINE_CIRCLE
};

typedef struct AppContext {
    const char* deviceName;
    const char* windowName;
    SDL_Window* window;
    SDL_GPUDevice* device;
    SDL_GPUGraphicsPipeline* builtinPipelines[APP_CONTEXT_BUILTIN_PIPELINES_MAX];
} AppContext;

int AppContextInit(AppContext* app_context, SDL_WindowFlags windowFlags);

int AppContextTerminate(AppContext* app_context);

#endif