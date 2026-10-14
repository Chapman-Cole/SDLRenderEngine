#include "ClayCompositor.h"
#include "DrawFunctions.h"

static inline GPUText ClayCompositorGetTextObject(ClayCompositor* clayCompositor, const char* text, uint32_t textLen, uint16_t fontID, float color[4]) {
    for (int i = 0; i < clayCompositor->textObjectCache.len; i++) {
        if (
            clayCompositor->textObjectCache.arr[i].textLen == textLen &&
            SDL_strncmp(clayCompositor->textObjectCache.arr[i].text.text->text, text, textLen * sizeof(char)) == 0
        ) {
            clayCompositor->textObjectCache.arr[i].taggedForDeletion = false;
            TTF_Font* font = FontRegistryGetWithKey(clayCompositor->fontRegistry, fontID);
            if (font != clayCompositor->textObjectCache.arr[i].text.font) {
                GPUTextUpdateFont(&clayCompositor->textObjectCache.arr[i].text, font);
            }
            return clayCompositor->textObjectCache.arr[i].text;
        }
    }

    TTF_Font* font = FontRegistryGetWithKey(clayCompositor->fontRegistry, fontID);
    GPUText gpuText;
    GPUTextCreate(&gpuText, clayCompositor->appContext, font, text, textLen, color);

    GPUTextInfo textInfo = {
        .text = gpuText,
        .textLen = textLen,
        .taggedForDeletion = false, // Newly allocated text objects will persist until the next frame to determine their permanence
    };

    DYNAMIC_ARRAY_APPEND(clayCompositor->textObjectCache, textInfo);

    return gpuText;
}

void ClayCompositorRenderCommands(ClayCompositor* clayCompositor, RenderContext* renderContext, Clay_RenderCommandArray* renderCommands) {
    for (int i = 0; i < clayCompositor->textObjectCache.len; i++) {
        clayCompositor->textObjectCache.arr[i].taggedForDeletion = true;
    }

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
            GPUText gpuText = ClayCompositorGetTextObject(clayCompositor, config->stringContents.chars, config->stringContents.length, config->fontId, textColor);
            // Must flush appContext's transfer context so that the text data makes it to the gpu in time
            APP_CONTEXT_FLUSH_TRANSFER_CONTEXT(*renderContext->appContext);
            DrawText(renderContext, &gpuText, (float[2]){bounding_box.x, bounding_box.y});
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

    for (int i = 0; i < clayCompositor->textObjectCache.len; i++) {
        if (clayCompositor->textObjectCache.arr[i].taggedForDeletion == true) {
            GPUTextDestroy(&clayCompositor->textObjectCache.arr[i].text);
            DYNAMIC_ARRAY_REMOVE(clayCompositor->textObjectCache, i);
            // Account for the fact that i will be auto incremented after this, but
            // removing the current element already places the next element at the current index
            i--;
        }
    }
}