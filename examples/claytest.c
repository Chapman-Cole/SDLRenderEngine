#define CLAY_IMPLEMENTATION
#define SDL_MAIN_USE_CALLBACKS
#include "clay.h"
#include "ClayCompositor.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "SDLRenderEngine.h"

#define RGBA_TO_F(r, g, b, a) ((float[4]){(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f})

// expects capital letters
#define CHAR_TO_HEX_NUMBER(c) (((char)(c) - 65) < 0 ? ((char)(c) - 48) : (10 + (char)(c) - 65))

// string must be at least 6 characters long (alpha should be a float between 0 and 255)
// expects capital letters
#define HEX_COLOR_TO_F(hexStr, alpha)                                       \
    RGBA_TO_F(                                                              \
        CHAR_TO_HEX_NUMBER(hexStr[1]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[0]), \
        CHAR_TO_HEX_NUMBER(hexStr[3]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[2]), \
        CHAR_TO_HEX_NUMBER(hexStr[5]) + 16 * CHAR_TO_HEX_NUMBER(hexStr[4]), \
        alpha)

Clay_Dimensions measureText(Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    FontRegistry* registry = (FontRegistry*)userData;
    TTF_Font* font = FontRegistryGetWithKey(registry, config->fontId);

    int width, height;
    TTF_GetStringSize(font, text.chars, text.length, &width, &height);

    return (Clay_Dimensions){
        .width = width,
        .height = height,
    };
}

void HandleClayErrors(Clay_ErrorData errorData) {
    SDL_Log("%s", errorData.errorText.chars);
    switch (errorData.errorType) {
    }
}

Clay_ElementDeclaration sidebarItemConfig = {
    .layout = {
        .sizing = {.width = {.size = {.minMax = {0}}, .type = CLAY__SIZING_TYPE_GROW}, .height = {.size = {.minMax = {50, 150}}, .type = CLAY__SIZING_TYPE_FIXED}},
        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
    },
    .backgroundColor = {27, 65, 85, 255},
    .cornerRadius = {7.0, 7.0, 7.0, 7.0},
};

void SidebarItemComponent(int id, int fontId) {
    CLAY(CLAY_IDI("sidebarItem", id), sidebarItemConfig) {
        CLAY_TEXT(CLAY_STRING("Compositor"), {.fontSize = 12, .fontId = fontId, .textColor = {255, 255, 255, 255}});
    }
}

#ifdef _WIN32
static const char* font_path = "C:\\WINDOWS\\FONTS\\SEGUIVAR.TTF";
#elif defined(__linux__)
static const char* font_path = "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf";
#endif

AppContext appContext = {0};
FontRegistry fontRegistry;
FontRegistryKey HackRegular12;
InputContext inputContext;
ClayCompositor clayCompositor;

SDL_Mutex* openFileMutex = NULL;
SDL_Mutex* openFontMutex = NULL;

char file_path[301];
int file_path_len = 0;

static void SDLCALL file_dialog_callback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        SDL_Log("Failed to get the file: %s", SDL_GetError());
        return;
    } else if (*filelist == NULL) {
        SDL_Log("No file selected");
        return;
    }

    SDL_LockMutex(openFileMutex);

    file_path_len = SDL_snprintf(file_path, 300, "%s", filelist[0]);
    file_path_len = SDL_clamp(file_path_len, 0, 300);

    SDL_UnlockMutex(openFileMutex);
}

char open_font_path[301];
int open_font_path_len = 0;

static void SDLCALL open_font_file_dialog_callback(void* userdata, const char* const* filelist, int filter) {
    if (!filelist) {
        SDL_Log("Failed to get the file: %s", SDL_GetError());
        return;
    } else if (*filelist == NULL) {
        SDL_Log("No file selected");
        return;
    }

    SDL_LockMutex(openFontMutex);
    open_font_path_len = SDL_snprintf(open_font_path, 300, "%s", filelist[0]);
    open_font_path_len = SDL_clamp(open_font_path_len, 0, 300);
    SDL_UnlockMutex(openFontMutex);
}

void open_font_hover_callback(Clay_ElementId elementId, Clay_PointerData pointerData, void* userdata) {
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        SDL_ShowOpenFileDialog(open_font_file_dialog_callback, NULL, appContext.window, NULL, 0, NULL, false);
    }
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    appContext.windowName = "ClayTest";
    appContext.deviceName = APP_CONTEXT_DEVICE_DRIVER_AGNOSTIC;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, false, 1'000'00);
    AppContextSetSwapchainParameters(&appContext, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);
    AppContextSetLatencyEasing(&appContext, 2);

    FontRegistryCreate(&fontRegistry, &appContext);
    FontRegistrySet(&fontRegistry, "Hack-Regular12", font_path, 14.0f);
    HackRegular12 = FontRegistryGenKey(&fontRegistry, "Hack-Regular12");

    InputContextCreate(&inputContext, &appContext, FontRegistryGetWithKey(&fontRegistry, HackRegular12), (float[4]){1.0, 1.0, 1.0, 1.0}, (float[4]){1.0, 1.0, 1.0, 1.0}, (InputContextLayout){.x = 0, .y = 0, .width = 100, .height = 100});
    InputContextSetLayoutMode(&inputContext, INPUT_CONTEXT_LAYOUT_MODE_SCROLLABLE);
    InputContextSetTabWidth(&inputContext, 4);

    ClayCompositorCreate(&clayCompositor, &appContext, &fontRegistry);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, SDL_malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions){640, 480}, (Clay_ErrorHandler){HandleClayErrors});
    Clay_SetMeasureTextFunction(measureText, &fontRegistry);

    SDL_StartTextInput(appContext.window);

    openFileMutex = SDL_CreateMutex();

    openFontMutex = SDL_CreateMutex();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    if (SDL_TryLockMutex(openFileMutex)) {
        if (file_path_len > 0) {
            file_path[file_path_len] = '\0';
            InputContextOpenFile(&inputContext, file_path);
            file_path_len = 0;
        }
        SDL_UnlockMutex(openFileMutex);
    }

    if (SDL_TryLockMutex(openFontMutex)) {
        if (open_font_path_len > 0) {
            open_font_path[open_font_path_len] = '\0';
            FontRegistrySet(&fontRegistry, open_font_path, open_font_path, 14.0f);
            InputContextSetFont(&inputContext, FontRegistryGet(&fontRegistry, open_font_path));
            open_font_path_len = 0;
        }
        SDL_UnlockMutex(openFontMutex);
    }

    AppContextUpdate(&appContext);
    InputContextRefresh(&inputContext);

    RenderContext renderContext = {0};
    renderContext.texture = NULL;

    if (RenderContextBegin(&renderContext, &appContext, NULL)) {

        Clay_SetLayoutDimensions((Clay_Dimensions){renderContext.renderWidth, renderContext.renderHeight});
        Clay_SetPointerState((Clay_Vector2){appContext.mousePixelX, appContext.mousePixelY}, appContext.isLeftMouseDown);

        Clay_BeginLayout();

        // Since context data must be passed through as a pointer, it must be created ouutside the macros
        // do avoid it being removed from the stack and causing a stack trace error. Basically,
        // a persistent pointer (just for the lifetime frame) is needed
        CompositorExtensionData inputContextData = {.type = COMPOSITOR_EXTENSION_TYPE_INPUT_CONTEXT, .inputContext = &inputContext};

        CLAY(CLAY_ID("OuterContainer"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = {41, 44, 48, 255}}) {
            CLAY(CLAY_ID("SideBar"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_PERCENT(0.25), .height = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .cornerRadius = {12.0, 12.0, 12.0, 12.0}, .backgroundColor = {25, 25, 25, 255}}) {
                CLAY(
                    CLAY_ID("OpenFontButton"),
                    {.layout = {
                         .sizing = {
                             .width = {
                                 .size = {.minMax = {0}},
                                 .type = CLAY__SIZING_TYPE_GROW},
                             .height = {.size = {.minMax = {50, 150}}, .type = CLAY__SIZING_TYPE_FIXED}},
                         .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                     },
                     .backgroundColor = Clay_Hovered() ? (Clay_Color){255, 116, 108, 255} : (Clay_Color){27, 65, 85, 255},
                     .cornerRadius = {7.0, 7.0, 7.0, 7.0}}) {
                    CLAY_TEXT(CLAY_STRING("Open Font"), {.fontSize = 12, .fontId = HackRegular12, .textColor = {255, 255, 255, 255}});
                    Clay_OnHover(open_font_hover_callback, NULL);
                }

                CLAY(CLAY_ID("SideBarElement1"), sidebarItemConfig) {
                    static char buff1[200];
                    int buffLen = SDL_snprintf(buff1, 200, "(%d,%d)", (int)(appContext.mousePixelX), (int)(appContext.mousePixelY));
                    CLAY_TEXT(((Clay_String){.chars = buff1, .length = buffLen, .isStaticallyAllocated = false}), {.fontSize = 12, .fontId = HackRegular12, .textColor = {255, 255, 255, 255}});
                }

                CLAY(CLAY_ID("SideBarElement3"), sidebarItemConfig) {
                    static char buff3[200] = {0};
                    static int buff3_len = 0;
                    static double buff3_fps = 0.0;
                    static int buff3_frame_counter = 0;
                    static double buff3_time_tracker = 0.0;

                    buff3_fps += 1.0 / appContext.deltaTime;
                    buff3_frame_counter++;
                    buff3_time_tracker += appContext.deltaTime;

                    if (buff3_time_tracker >= 0.1) {
                        buff3_len = SDL_snprintf(buff3, 200, "FPS: %d", (int)(buff3_fps / (double)buff3_frame_counter));
                        buff3_frame_counter = 0;
                        buff3_time_tracker = 0.0;
                        buff3_fps = 0.0;
                    }

                    CLAY_TEXT(((Clay_String){.chars = buff3, .length = buff3_len, .isStaticallyAllocated = false}), {.fontSize = 12, .fontId = HackRegular12, .textColor = {255, 255, 255, 255}});
                }

                CLAY(CLAY_ID("SideBarElement4"), sidebarItemConfig) {
                    static char buff4[200] = {0};
                    static int buff4_len = 0;

                    buff4_len = SDL_snprintf(buff4, 200, "(%d,%d)", (int)(renderContext.renderWidth), (int)(renderContext.renderHeight));

                    CLAY_TEXT(((Clay_String){.chars = buff4, .length = buff4_len, .isStaticallyAllocated = false}), {.fontSize = 12, .fontId = HackRegular12, .textColor = {255, 255, 255, 255}});
                }

                CLAY(CLAY_ID("SideBarElement5"), sidebarItemConfig) {
                    static char buff5[200] = {0};
                    static int buff5_len = 0;

                    buff5_len = SDL_snprintf(buff5, 200, "line %d\nlines: %d", inputContext.cursorLine, inputContext.buffer.len);

                    CLAY_TEXT(((Clay_String){.chars = buff5, .length = buff5_len, .isStaticallyAllocated = false}), {.fontSize = 12, .fontId = HackRegular12, .textColor = {255, 255, 255, 255}});
                }
            }

            CLAY(CLAY_ID("MainContent"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16)}, .cornerRadius = {12.0, 12.0, 12.0, 12.0}, .backgroundColor = {25, 25, 25, 255}}) {
                CLAY(CLAY_ID("InputArea"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .custom = {.customData = &inputContextData}});
            }
        }

        Clay_RenderCommandArray clayRenderCommands = Clay_EndLayout(appContext.deltaTime);
        ClayCompositorRenderCommands(&clayCompositor, &renderContext, &clayRenderCommands);

        RenderContextEnd(&renderContext);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        const bool* key_states = SDL_GetKeyboardState(NULL);

        switch (event->key.key) {
        case SDLK_L: {
            if (key_states[SDL_SCANCODE_LCTRL] == true) {
                InputContextClear(&inputContext);
            }
        } break;

        case SDLK_O: {
            if (key_states[SDL_SCANCODE_LCTRL] == true) {
                SDL_ShowOpenFileDialog(file_dialog_callback, NULL, appContext.window, NULL, 0, NULL, false);
            }
        } break;
        }
    }

    InputContextProcessEvent(&inputContext, event);

    AppContextProcessEvent(&appContext, event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_DestroyMutex(openFontMutex);
    SDL_DestroyMutex(openFileMutex);

    SDL_StopTextInput(appContext.window);

    ClayCompositorDestroy(&clayCompositor);

    FontRegistryDestroy(&fontRegistry);

    InputContextDestroy(&inputContext);

    AppContextTerminate(&appContext);
}