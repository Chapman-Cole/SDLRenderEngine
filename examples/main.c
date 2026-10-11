#include "SDL3/SDL.h"
#include "SDLRenderEngine.h"
#include "stdlib.h"

static float vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f};

static Uint16 indices[] = {0, 1, 2, 1, 3, 2};

#ifdef _WIN32
    static const char* font_path = "C:\\WINDOWS\\FONTS\\SEGUIVAR.TTF";
#elif defined(__linux__)
    static const char* font_path = "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf";
#endif

int main(int argc, char* argv[]) {
    AppContext appContext = {0};
    appContext.windowName = "Renderer";
    appContext.deviceName = NULL;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, true);
    SDL_SetGPUSwapchainParameters(appContext.device, appContext.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    FontRegistry fontRegistry;
    FontRegistryCreate(&fontRegistry, &appContext);
    FontRegistrySet(&fontRegistry, "Hack-Regular15", font_path, 15.0f);

    SDL_StartTextInput(appContext.window);

    InputContext inputContext;
    InputContextCreate(&inputContext, &appContext, FontRegistryGet(&fontRegistry, "Hack-Regular15"), (float[4]){1.0, 1.0, 1.0, 1.0f}, (float[4]){1.0, 1.0, 1.0, 1.0f}, (InputContextLayout){.x = 100.0f, .y = 100.0f, .width = 500.0f, .height = 1000.0f});

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

        if (RenderContextBegin(&renderContext, &appContext, NULL) < 0) {
            exit(-1);
        }

        DrawInputContext(&renderContext, &inputContext);

        RenderContextEnd(&renderContext);
    }

    InputContextDestroy(&inputContext);

    SDL_StopTextInput(appContext.window);

    FontRegistryDestroy(&fontRegistry);

    AppContextTerminate(&appContext);
    return 0;
}
