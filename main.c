#include "AppContext.h"
#include "DrawFunctions.h"
#include "FontRegistry.h"
#include "GPUBuffers.h"
#include "GPUText.h"
#include "InputContext.h"
#include "RenderContext.h"
#include "Shaders.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_textengine.h>
#include <SDL3_ttf/SDL_ttf.h>

static float vertices[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f};

static Uint16 indices[] = {
    0, 1, 2,
    1, 3, 2};

int main(int argc, char* argv[]) {
    AppContext appContext = {0};
    appContext.windowName = "Renderer";
    appContext.deviceName = NULL;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_SetGPUSwapchainParameters(appContext.device, appContext.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    FontRegistry fontRegistry;
    FontRegistryCreate(&fontRegistry, &appContext);
    FontRegistrySet(&fontRegistry, "Hack-Regular15", "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf", 15.0f);

    SDL_StartTextInput(appContext.window);

    float scaleFactor = 0.0f;

    bool equalsPrevDown = false;
    bool minusPrevDown = false;

    InputContext inputContext;
    InputContextCreate(&inputContext, &appContext, FontRegistryGet(&fontRegistry, "Hack-Regular15"));

    bool shouldContinue = true;
    while (shouldContinue) {
        SDL_Event event;
        InputContextBegin(&inputContext);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                shouldContinue = false;
                continue;
            }

            InputContextUpdate(&inputContext, &event);
        }
        InputContextEnd(&inputContext);

        RenderContext renderContext = {0};
        renderContext.texture = NULL;
        RenderContextBegin(&renderContext, &appContext, NULL);

        DrawInputContext(&renderContext, &inputContext, (float[]){0.0, 0.0});

        RenderContextEnd(&renderContext);
    }

    InputContextDestroy(&inputContext);

    SDL_StopTextInput(appContext.window);

    FontRegistryDestroy(&fontRegistry);

    AppContextTerminate(&appContext);
    return 0;
}