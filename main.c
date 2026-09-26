#include "AppContext.h"
#include "DrawFunctions.h"
#include "FontRegistry.h"
#include "GPUBuffers.h"
#include "GPUText.h"
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

    GPUText inputText;
    GPUTextCreate(&inputText, &appContext, FontRegistryGet(&fontRegistry, "Hack-Regular15"), "");

    int logicalWindowWidth, logicalWindowHeight;
    SDL_GetWindowSize(appContext.window, &logicalWindowWidth, &logicalWindowHeight);

    SDL_StartTextInput(appContext.window);

    float scaleFactor = 0.0f;

    bool equalsPrevDown = false;
    bool minusPrevDown = false;

    int inputBuffTracker = 0;
    char inputBuff[2000];

    bool shouldContinue = true;
    while (shouldContinue) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                shouldContinue = false;
                continue;
            } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (inputBuffTracker + 2 >= 2000) {
                    inputBuffTracker = 0;
                    SDL_zero(inputBuff);
                }

                inputBuff[inputBuffTracker] = event.text.text[0];
                inputBuff[inputBuffTracker + 1] = '\0';

                GPUTextUpdate(&inputText, inputBuff);
                inputBuffTracker++;
            }
        }

        RenderContext renderContext = {0};
        renderContext.texture = NULL;
        RenderContextBegin(&renderContext, &appContext, NULL);

        DrawText(&renderContext, &inputText, (float[]){0, 0});

        RenderContextEnd(&renderContext);
    }

    SDL_StopTextInput(appContext.window);

    GPUTextDestroy(&inputText);

    FontRegistryDestroy(&fontRegistry);

    AppContextTerminate(&appContext);
    return 0;
}