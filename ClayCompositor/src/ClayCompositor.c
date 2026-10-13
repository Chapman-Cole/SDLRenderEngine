#include "ClayCompositor.h"
#include "DrawFunctions.h"

static inline int ClayCompositorAppendTextObject(ClayCompositor* clayCompositor, const char* text, uint32_t textLen, uint16_t fontID, float color[4]) {
    if (clayCompositor->textObjectsLen + 1 >= clayCompositor->textObjectsCapacity) {
        clayCompositor->textObjectsCapacity += CLAY_COMPOSITOR_TEXT_OBJECT_ARRAY_GROW_AMOUNT;
        void* temp_ptr = SDL_realloc(clayCompositor->textObjects, clayCompositor->textObjectsCapacity * sizeof(GPUText));
        if (temp_ptr == NULL) {
            SDL_Log("Failed to allocate memory for GPUText objects in clay compositor.\n");
            return -1;
        }
        clayCompositor->textObjects = temp_ptr;
    }

    TTF_Font* font = FontRegistryGetWithKey(clayCompositor->fontRegistry, fontID);
    GPUTextCreate(&clayCompositor->textObjects[clayCompositor->textObjectsLen], clayCompositor->appContext, font, text, textLen, color);
    clayCompositor->textObjectsLen++;
    return 0;
}

void ClayCompositorRenderCommands(ClayCompositor* clayCompositor, RenderContext* renderContext, Clay_RenderCommandArray* renderCommands) {
    for (uint32_t i = 0; i < clayCompositor->textObjectsLen; i++) {
        GPUTextDestroy(&clayCompositor->textObjects[i]);
    }
    clayCompositor->textObjectsLen = 0;

    for (size_t i = 0; i < renderCommands->length; i++) {
        Clay_RenderCommand* command = Clay_RenderCommandArray_Get(renderCommands, i);
        const Clay_BoundingBox bounding_box = command->boundingBox;

        switch (command->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            Clay_RectangleRenderData* config = &command->renderData.rectangle;

            DrawRectangleRounded(
                renderContext,
                (float[2]){
                    bounding_box.x,
                    bounding_box.y,
                },
                (float[2]){
                    bounding_box.width,
                    bounding_box.height,
                },
                (float[4]){
                    config->backgroundColor.r / 255.0f,
                    config->backgroundColor.g / 255.0f,
                    config->backgroundColor.b / 255.0f,
                    config->backgroundColor.a / 255.0f,
                },
                (float[4]){
                    config->cornerRadius.topLeft,
                    config->cornerRadius.topRight,
                    config->cornerRadius.bottomLeft,
                    config->cornerRadius.bottomRight,
                });
        } break;

        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_TextRenderData* config = &command->renderData.text;
            float textColor[4] = {config->textColor.r / 255.0f, config->textColor.g / 255.0f, config->textColor.b / 255.0f, config->textColor.a / 255.0f};
            ClayCompositorAppendTextObject(clayCompositor, config->stringContents.chars, config->stringContents.length, config->fontId, textColor);
            APP_CONTEXT_FLUSH_TRANSFER_CONTEXT(*clayCompositor->appContext);
            DrawText(renderContext, &clayCompositor->textObjects[clayCompositor->textObjectsLen - 1], (float[2]){bounding_box.x, bounding_box.y});
        } break;

        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            CompositorExtensionData* extensionData = command->renderData.custom.customData;
            if (extensionData == NULL) {
                return;
            }

            switch (extensionData->type) {
            case COMPOSITOR_EXTENSION_TYPE_INPUT_CONTEXT: {
                extensionData->inputContext->layout.x = bounding_box.x;
                extensionData->inputContext->layout.y = bounding_box.y;
                extensionData->inputContext->layout.width = bounding_box.width;
                extensionData->inputContext->layout.height = bounding_box.height;
                DrawInputContext(renderContext, extensionData->inputContext);
            } break;
            }
        }
        }
    }
}