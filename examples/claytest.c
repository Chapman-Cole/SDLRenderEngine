#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "ClayCompositor.h"
#include "SDLRenderEngine.h"

const Clay_Color COLOR_LIGHT = (Clay_Color){224, 215, 210, 255};
const Clay_Color COLOR_RED = (Clay_Color){168, 66, 28, 255};
const Clay_Color COLOR_ORANGE = (Clay_Color){225, 138, 50, 255};

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
Clay_ElementDeclaration sidebarItemConfig = (Clay_ElementDeclaration){
    .layout = {
        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(50)}, .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}},
    .backgroundColor = COLOR_ORANGE,
    .cornerRadius = {2.0, 2.0, 2.0, 2.0},
};

// Re-useable components are just normal functions
void SidebarItemComponent(int id, int fontId) {
    CLAY(CLAY_IDI("sidebarItem", id), sidebarItemConfig) {
        CLAY_TEXT(CLAY_STRING("Compositor"), {.fontSize = 12, .fontId = fontId, .textColor = {0, 0, 0, 255}});
    }
}

int main(int argc, char* argv[]) {
    AppContext appContext = {0};
    appContext.windowName = "ClayTest";
    appContext.deviceName = NULL;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, false);
    SDL_SetGPUSwapchainParameters(appContext.device, appContext.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    FontRegistry fontRegistry;
    FontRegistryCreate(&fontRegistry, &appContext);
    FontRegistrySet(&fontRegistry, "Hack-Regular12", "/usr/share/fonts/source-foundry-hack-fonts/Hack-Regular.ttf", 12.0f);
    FontRegistryKey HackRegular12 = FontRegistryGenKey(&fontRegistry, "Hack-Regular12");

    InputContext inputContext;
    InputContextCreate(&inputContext, &appContext, FontRegistryGetWithKey(&fontRegistry, HackRegular12), (float[4]){0.0, 0.0, 0.0, 1.0}, (float[4]){0.0, 0.0, 0.0, 1.0}, (InputContextLayout){.x = 0, .y = 0, .width = 100.0f, .height = 100.0f});

    ClayCompositor clayCompositor;
    ClayCompositorCreate(&clayCompositor, &appContext, &fontRegistry);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, SDL_malloc(totalMemorySize));

    Clay_Initialize(arena, (Clay_Dimensions){640, 480}, (Clay_ErrorHandler){HandleClayErrors});
    Clay_SetMeasureTextFunction(measureText, &fontRegistry);

    float prevTime = (float)SDL_GetTicks() / 1000.0f;

    SDL_StartTextInput(appContext.window);

    bool shouldContinue = true;
    while (shouldContinue) {
        InputContextBegin(&inputContext);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                shouldContinue = false;
                continue;
            }

            InputContextUpdate(&inputContext, &event);
        }
        InputContextEnd(&inputContext);

        float currTime = (float)SDL_GetTicks() / 1000.0f;
        float deltaTime = currTime - prevTime;
        prevTime = currTime;

        RenderContext renderContext = {0};
        renderContext.texture = NULL;
        RenderContextBegin(&renderContext, &appContext, NULL);

        Clay_SetLayoutDimensions((Clay_Dimensions){renderContext.renderWidth, renderContext.renderHeight});

        Clay_BeginLayout();

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
                CLAY(CLAY_ID("InputArea"), {.layout = {.sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}}, .custom = {.customData = &(CompositorExtensionData){.type = COMPOSITOR_EXTENSION_TYPE_INPUT_CONTEXT, .inputContext = &inputContext}}});
            }
        }

        Clay_RenderCommandArray clayRenderCommands = Clay_EndLayout(deltaTime);
        ClayCompositorRenderCommands(&clayCompositor, &renderContext, &clayRenderCommands);

        RenderContextEnd(&renderContext);
    }

    SDL_StopTextInput(appContext.window);

    ClayCompositorDestroy(&clayCompositor);

    FontRegistryDestroy(&fontRegistry);

    InputContextDestroy(&inputContext);

    AppContextTerminate(&appContext);
    return 0;
}