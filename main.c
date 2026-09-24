#include "AppContext.h"
#include "DrawFunctions.h"
#include "GPUBuffers.h"
#include "RenderContext.h"
#include "Shaders.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_ttf/SDL_textengine.h>
#include "GPUText.h"

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

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/open-sans/OpenSans-Regular.ttf", 50.0);
    if (font == NULL) {
        SDL_Log("Failed to load font: %s", SDL_GetError());
        return -1;
    }

    GPUText text;
    GPUTextCreate(&text, &appContext, font, "Hello World!");

    bool shouldContinue = true;
    while (shouldContinue) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                shouldContinue = false;
                continue;
            }
        }


        RenderContext renderContext = {0};
        renderContext.texture = NULL;
        RenderContextBegin(&renderContext, &appContext, NULL);

        DrawText(&renderContext, &text);

        RenderContextEnd(&renderContext);
    }

    GPUTextDestroy(&text);

    AppContextTerminate(&appContext);
    return 0;
}