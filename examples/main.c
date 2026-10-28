
#define SDL_MAIN_USE_CALLBACKS
#include "HashMap.h"
#include "MemoryHeap.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "SDLRenderEngine.h"
#include "stdlib.h"

#ifdef _WIN32
static char font_path[256] = "C:\\WINDOWS\\FONTS\\CONSOLA.TTF";
#elif defined(__linux__)
static char font_path[256] = "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf";
#endif
static char fontNameNoExtension[256];
static bool fontWasUpdated = false;
static float file_dialog_font_size = 15.0f;

static AppContext appContext = {0};
static FontRegistry fontRegistry;
static InputContext inputContext;
static GPUText fpsText;

SDL_Mutex* fontMutex;

static const SDL_DialogFileFilter dialogFilters[] = {
    {"TTF Files", "ttf"},
    {"OTF Files", "otf"},
};

// This code is just based on the example from the SDL3 wiki: https://wiki.libsdl.org/SDL3/SDL_DialogFileCallback
static void SDLCALL file_dialog_callback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        SDL_Log("Failed to get font file: %s", SDL_GetError());
        return;
    } else if (*filelist == NULL) {
        SDL_Log("No file selected.\n");
        return;
    }

    if (filter < 0) {
        SDL_Log("Unsupported file type.\n");
        return;
    }

    const char* file = filelist[0];
    int file_len = SDL_strlen(file);
    int lastSlashIndex = -1; // -1 so that in the case there are no slashes, lastSlashIndex + 1 just evaluates to 0 in SDL_snprintf and the whole string is printed
    int lastPeriodIndex = file_len;
    for (int i = 0; i < file_len; i++) {
        if (file[i] == '\\' || file[i] == '/') {
            lastSlashIndex = i;
        } else if (file[i] == '.') {
            lastPeriodIndex = i;
        }
    }

    file_dialog_font_size = 15.0f;

    SDL_LockMutex(fontMutex);
    fontWasUpdated = true;
    SDL_snprintf(fontNameNoExtension, sizeof(fontNameNoExtension)/sizeof(fontNameNoExtension[0]), "%.*s%0.1f", lastPeriodIndex - (lastSlashIndex + 1), &file[lastSlashIndex + 1], file_dialog_font_size);
    SDL_strlcpy(font_path, file, sizeof(font_path)/sizeof(font_path[0]));
    SDL_UnlockMutex(fontMutex);
}

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

    fontMutex = SDL_CreateMutex();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    if (SDL_TryLockMutex(fontMutex)) {
        if (fontWasUpdated == true) {
            fontWasUpdated = false;
            FontRegistrySet(&fontRegistry, fontNameNoExtension, font_path, file_dialog_font_size);
            InputContextSetFont(&inputContext, FontRegistryGet(&fontRegistry, fontNameNoExtension));
        }
        SDL_UnlockMutex(fontMutex);
    }

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
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        const bool* key_states = SDL_GetKeyboardState(NULL);
        switch (event->key.key) {
        case SDLK_O:
            if (key_states[SDL_SCANCODE_LCTRL] == true) {
                SDL_ShowOpenFileDialog(
                    file_dialog_callback,
                    NULL,
                    appContext.window, dialogFilters, 2, NULL, false);
            }
            break;
        }
    }

    InputContextProcessEvent(&inputContext, event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_DestroyMutex(fontMutex);

    GPUTextDestroy(&fpsText);

    InputContextDestroy(&inputContext);

    SDL_StopTextInput(appContext.window);

    FontRegistryDestroy(&fontRegistry);

    AppContextTerminate(&appContext);
}
