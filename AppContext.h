#ifndef APPCONTEXT_H
#define APPCONTEXT_H

#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "SDL3_ttf/SDL_textengine.h"

#define APP_CONTEXT_BUILTIN_PIPELINES_MAX 12

enum BuiltinPipelineIndex {
    BUILTIN_PIPELINE_RECTANGLE,
    BUILTIN_PIPELINE_CIRCLE,
    BUILTIN_PIPELINE_TEXT
};

typedef struct AppContext {
    const char* deviceName;
    const char* windowName;
    SDL_Window* window;
    SDL_GPUDevice* device;
    TTF_TextEngine* textEngine;
    SDL_GPUGraphicsPipeline* builtinPipelines[APP_CONTEXT_BUILTIN_PIPELINES_MAX];
} AppContext;

int AppContextInit(AppContext* app_context, SDL_WindowFlags windowFlags);

int AppContextTerminate(AppContext* app_context);

#endif