#define CLAY_IMPLEMENTATION
#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "clay.h"
#include "ClayCompositor.h"
#include "SDLRenderEngine.h"

#define COLOR_LIGHT {224, 215, 210, 255}
#define COLOR_RED {168, 66, 28, 255}
#define COLOR_ORANGE {225, 138, 50, 255}

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

// Layout config is just a struct that can be declared statically, or inline
Clay_ElementDeclaration sidebarItemConfig = {
    .layout = {
        .sizing = {.width = {.size = {.minMax = {0}}, .type = CLAY__SIZING_TYPE_GROW}, .height = {.size = {.minMax = {50, 50}}, .type = CLAY__SIZING_TYPE_FIXED}}, .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}},
    .backgroundColor = COLOR_ORANGE,
    .cornerRadius = {2.0, 2.0, 2.0, 2.0},
};

// Re-useable components are just normal functions
void SidebarItemComponent(int id, int fontId) {
    CLAY(CLAY_IDI("sidebarItem", id), sidebarItemConfig) {
        CLAY_TEXT(CLAY_STRING("Compositor"), {.fontSize = 12, .fontId = fontId, .textColor = {0, 0, 0, 255}});
    }
}

AppContext appContext = {0};
FontRegistry fontRegistry;
FontRegistryKey HackRegular12;
InputContext inputContext;
ClayCompositor clayCompositor;
float prevTime = 0.0f;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
    appContext.windowName = "ClayTest";
    appContext.deviceName = NULL;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, false);
    SDL_SetGPUSwapchainParameters(appContext.device, appContext.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    FontRegistryCreate(&fontRegistry, &appContext);
    FontRegistrySet(&fontRegistry, "Hack-Regular12", "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf", 12.0f);
    HackRegular12 = FontRegistryGenKey(&fontRegistry, "Hack-Regular12");

    InputContextCreate(&inputContext, &appContext, FontRegistryGetWithKey(&fontRegistry, HackRegular12), (float[4]){0.0, 0.0, 0.0, 1.0}, (float[4]){0.0, 0.0, 0.0, 1.0}, (InputContextLayout){.x = 0, .y = 0, .width = 100.0f, .height = 100.0f});

    ClayCompositorCreate(&clayCompositor, &appContext, &fontRegistry);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, SDL_malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions){640, 480}, (Clay_ErrorHandler){HandleClayErrors});
    Clay_SetMeasureTextFunction(measureText, &fontRegistry);

    float prevTime = (float)SDL_GetTicks() / 1000.0f;

    SDL_StartTextInput(appContext.window);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    float currTime = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = currTime - prevTime;
    prevTime = currTime;

    RenderContext renderContext = {0};
    renderContext.texture = NULL;
    RenderContextBegin(&renderContext, &appContext, NULL);

    Clay_SetLayoutDimensions((Clay_Dimensions){renderContext.renderWidth, renderContext.renderHeight});

    Clay_BeginLayout();

    // Since context data must be passed through as a pointer, it must be created ouutside the macros
    // do avoid it being removed from the stack and causing a stack trace error. Basically,
    // a persistent pointer (just for the lifetime frame) is needed
    CompositorExtensionData inputContextData = {.type = COMPOSITOR_EXTENSION_TYPE_INPUT_CONTEXT, .inputContext = &inputContext };

    CLAY(CLAY_ID("OuterContainer"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = {250, 250, 255, 255}}) {
        CLAY(CLAY_ID("SideBar"), {.layout = {.layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {.width = CLAY_SIZING_PERCENT(0.25), .height = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16}, .backgroundColor = COLOR_LIGHT}) {
            CLAY(CLAY_ID("SideBarElement1"), sidebarItemConfig) {
                static char buff1[200];
                int buffLen = SDL_snprintf(buff1, 200, "(%.1f,%.1f)", inputContext.layout.width, inputContext.layout.height);
                CLAY_TEXT(((Clay_String){.chars = buff1, .length = buffLen, .isStaticallyAllocated = false}), {.fontSize = 12, .fontId = HackRegular12, .textColor = {0, 0, 0, 255}});
            }

            CLAY(CLAY_ID("SideBarElement2"), sidebarItemConfig) {
                static char buff2[200];
                int textWidth, textHeight;
                TTF_GetStringSize(FontRegistryGetWithKey(&fontRegistry, HackRegular12), inputContext.gpuText.text->text, 0, &textWidth, &textHeight);
                int buffLen = SDL_snprintf(buff2, 200, "(%d,%d)", textWidth, textHeight);
                CLAY_TEXT(((Clay_String){.chars = buff2, .length = buffLen, .isStaticallyAllocated = false}), {.fontSize = 12, .fontId = HackRegular12, .textColor = {0, 0, 0, 255}});
            }
        }

        CLAY(CLAY_ID("MainContent"), {.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16)}, .backgroundColor = COLOR_LIGHT}) {
            CLAY(CLAY_ID("InputArea"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .custom = {.customData = &inputContextData}});
        }
    }

    Clay_RenderCommandArray clayRenderCommands = Clay_EndLayout(deltaTime);
    ClayCompositorRenderCommands(&clayCompositor, &renderContext, &clayRenderCommands);

    RenderContextEnd(&renderContext);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    InputContextBegin(&inputContext);
    InputContextUpdate(&inputContext, event);
    InputContextEnd(&inputContext);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_StopTextInput(appContext.window);

    ClayCompositorDestroy(&clayCompositor);

    FontRegistryDestroy(&fontRegistry);

    InputContextDestroy(&inputContext);

    AppContextTerminate(&appContext);
}