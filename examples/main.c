
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "SDLRenderEngine.h"
#include "stdlib.h"

#ifdef _WIN32
static const char* font_path = "C:\\WINDOWS\\FONTS\\SEGUIVAR.TTF";
#elif defined(__linux__)
static const char* font_path = "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf";
#endif

static AppContext appContext = {0};
static FontRegistry fontRegistry;
static InputContext inputContext;
static GPUText fpsText;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    appContext.windowName = "Renderer";
    appContext.deviceName = APP_CONTEXT_DEVICE_DRIVER_AGNOSTIC;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, false, 1'000'000);
    SDL_SetGPUSwapchainParameters(appContext.device, appContext.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    FontRegistryCreate(&fontRegistry, &appContext);
    FontRegistrySet(&fontRegistry, "Hack-Regular15", font_path, 15.0f);

    SDL_StartTextInput(appContext.window);

    InputContextCreate(&inputContext, &appContext, FontRegistryGet(&fontRegistry, "Hack-Regular15"), (float[4]){1.0, 1.0, 1.0, 1.0f}, (float[4]){1.0, 1.0, 1.0, 1.0f}, (InputContextLayout){.x = 100.0f, .y = 100.0f, .width = 2000.0f, .height = 1000.0f});

    GPUTextCreate(&fpsText, &appContext, FontRegistryGet(&fontRegistry, "Hack-Regular15"), "FPS:", 0, (float[4]){1.0, 1.0, 1.0, 1.0});

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    AppContextUpdate(&appContext);
    InputContextRefresh(&inputContext);

    static char buff[200] = {0};
    static int buffLen = 0;
    static double fps_counter = 0.0;
    static int frame_counter = 0;
    static double fps_time_tracker = 0.0;

    fps_counter += 1.0 / appContext.deltaTime;
    fps_time_tracker += appContext.deltaTime;
    frame_counter++;

    if (fps_time_tracker >= 0.2) {
        SDL_snprintf(buff, 200, "FPS: %d", (int)(fps_counter / (double)frame_counter));
        
        GPUTextUpdate(&fpsText, buff, buffLen);

        fps_counter = 0.0;
        frame_counter = 0;
        fps_time_tracker = 0.0;
    }

    RenderContext renderContext = {0};
    renderContext.texture = NULL;

    if (RenderContextBegin(&renderContext, &appContext, NULL) < 0) {
        exit(-1);
    }

    DrawText(&renderContext, &fpsText, (float[2]){renderContext.renderWidth - fpsText.width, renderContext.renderHeight - fpsText.height});
    DrawInputContext(&renderContext, &inputContext);

    RenderContextEnd(&renderContext);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    InputContextProcessEvent(&inputContext, event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    GPUTextDestroy(&fpsText);

    InputContextDestroy(&inputContext);

    SDL_StopTextInput(appContext.window);

    FontRegistryDestroy(&fontRegistry);

    AppContextTerminate(&appContext);
}
