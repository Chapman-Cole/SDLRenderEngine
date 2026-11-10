#include "DrawFunctions.h"
#include "InputContext.h"

typedef struct float2 {
    float x;
    float y;
} float2;

typedef struct float3 {
    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };
    union {
        float z;
        float b;
    };
} float3;

typedef struct float4 {
    union {
        float x;
        float r;
    };
    union {
        float y;
        float g;
    };
    union {
        float z;
        float b;
    };
    union {
        float w;
        float a;
    };
} float4;

typedef struct TextVertexUniform {
    float2 size;
    float2 position;
    float2 windowDims;
    float2 pad;
} TextVertexUniform;

typedef struct TextFragmentUniform {
    float4 color;
    TextRenderInfoImageType imageType;
    float3 pad;
} TextFragmentUniform;

typedef struct RectangleVertexUniform {
    float2 position;
    float2 size;
    float2 windowDims;
    float2 pad;
} RectangleVertexUniform;

typedef struct RectangleFragmentUniform {
    float4 color;
    float2 position;
    float2 size;
    float4 cornerRadii;
} RectangleFragmentUniform;

typedef struct CircleVertexUniform {
    float2 center;
    float2 windowDims;
    float radius;
    float aspectRatio;
    float2 pad;
} CircleVertexUniform;

typedef struct CircleFragmentUniform {
    float4 color;
    float2 center;
    float radius;
    float aspectRatio;
} CircleFragmentUniform;

typedef struct TextCursorVertexUniform {
    float2 position;
    float2 size;
    float2 windowDims;
    float2 pad;
} TextCursorVertexUniform;

typedef struct TextCursorFragmentUniform {
    float4 color;
} TextCursorFragmentUniform;

void DrawRectangle(RenderContext* renderContext, float position[2], float size[2], float color[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);

    RectangleVertexUniform vertexUniform = (RectangleVertexUniform){
        .position = (float2){position[0], position[1]},
        .size = (float2){size[0], size[1]},
        .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
        .pad = {0},
    };

    SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &vertexUniform, sizeof(vertexUniform));

    RectangleFragmentUniform fragmentUniform = (RectangleFragmentUniform){
        .color = (float4){color[0], color[1], color[2], color[3]},
        .position = (float2){position[0], position[1]},
        .size = (float2){size[0], size[1]},
        .cornerRadii = (float4){0.0f, 0.0f, 0.0f, 0.0f},
    };

    SDL_PushGPUFragmentUniformData(renderContext->cmdbuf, 0, &fragmentUniform, sizeof(fragmentUniform));

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawRectangleRounded(RenderContext* renderContext, float position[2], float size[2], float color[4], float cornerRadii[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);

    RectangleVertexUniform vertexUniform = (RectangleVertexUniform){
        .position = (float2){position[0], position[1]},
        .size = (float2){size[0], size[1]},
        .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
        .pad = {0},
    };

    SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &vertexUniform, sizeof(vertexUniform));

    RectangleFragmentUniform fragmentUniform = (RectangleFragmentUniform){
        .color = (float4){color[0], color[1], color[2], color[3]},
        .position = (float2){position[0], position[1]},
        .size = (float2){size[0], size[1]},
        .cornerRadii = (float4){cornerRadii[0], cornerRadii[1], cornerRadii[2], cornerRadii[3]},
    };

    SDL_PushGPUFragmentUniformData(renderContext->cmdbuf, 0, &fragmentUniform, sizeof(fragmentUniform));

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawCircle(RenderContext* renderContext, float center[2], float radius, float color[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_CIRCLE]);

    CircleVertexUniform vertexUniform = (CircleVertexUniform){
        .center = (float2){center[0], center[1]},
        .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
        .radius = radius,
        .aspectRatio = renderContext->aspectRatio,
        .pad = {0},
    };

    SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &vertexUniform, sizeof(vertexUniform));

    CircleFragmentUniform fragmentUniform = (CircleFragmentUniform){
        .color = (float4){color[0], color[1], color[2], color[3]},
        .center = (float2){center[0], center[1]},
        .radius = radius,
        .aspectRatio = renderContext->aspectRatio,
    };

    SDL_PushGPUFragmentUniformData(renderContext->cmdbuf, 0, &fragmentUniform, sizeof(fragmentUniform));

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawText(RenderContext* renderContext, GPUText* text, float position[2]) {
    // If the number of lines is 0, then just skip rendering entirely
    if (text->text->num_lines == 0) {
        return;
    }

    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT]);

    TextVertexUniform vertexUniform = (TextVertexUniform){
        .size = (float2){(float)text->width, (float)text->height},
        .position = (float2){position[0], position[1]},
        .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
        .pad = {0},
    };

    SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &vertexUniform, sizeof(vertexUniform));

    TextFragmentUniform fragmentUniform = (TextFragmentUniform){
        .color = (float4){text->color[0], text->color[1], text->color[2], text->color[3]},
    };

    SDL_PushGPUFragmentUniformData(renderContext->cmdbuf, 0, &fragmentUniform, sizeof(fragmentUniform));

    for (Uint32 i = 0; i < text->numAtlasses; i++) {

        SDL_BindGPUVertexBuffers(
            renderContext->renderPass,
            0,
            (SDL_GPUBufferBinding[]){{
                .buffer = text->atlasses[i].vertexBuffer->buffer,
                .offset = 0,
            }},
            1);

        SDL_BindGPUIndexBuffer(
            renderContext->renderPass,
            &(SDL_GPUBufferBinding){
                .buffer = text->atlasses[i].indexBuffer->buffer,
                .offset = 0,
            },
            SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_BindGPUFragmentSamplers(
            renderContext->renderPass,
            0,
            (SDL_GPUTextureSamplerBinding[]){{
                .sampler = renderContext->appContext->textSampler,
                .texture = text->atlasses[0].atlas,
            }},
            1);

        SDL_DrawGPUIndexedPrimitives(renderContext->renderPass, text->atlasses[i].numIndices, 1, 0, 0, 0);
    }
}

void DrawInputContext(RenderContext* renderContext, InputContext* inputContext) {
    bool scissorStateWasUpdated = false;
    switch (inputContext->layoutMode) {
    case INPUT_CONTEXT_LAYOUT_MODE_UNBOUNDED: {
        // No need to set gpu scissor
    } break;

    case INPUT_CONTEXT_LAYOUT_MODE_BOUNDED_CUTOFF: {
        scissorStateWasUpdated = true;
        SDL_SetGPUScissor(
            renderContext->renderPass,
            &(SDL_Rect){
                .x = (int)inputContext->layout.x,
                .y = (int)inputContext->layout.y,
                .w = (int)inputContext->layout.width,
                .h = (int)inputContext->layout.height,
            });
    } break;

    case INPUT_CONTEXT_LAYOUT_MODE_SCROLLABLE: {
        scissorStateWasUpdated = true;
        SDL_SetGPUScissor(
            renderContext->renderPass,
            &(SDL_Rect){
                .x = (int)inputContext->layout.x,
                .y = (int)inputContext->layout.y,
                .w = (int)inputContext->layout.width,
                .h = (int)inputContext->layout.height,
            });

    } break;

    default: {
        // By default don't set gpu scissor state
    } break;
    }

    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT]);

    int offsetY = inputContext->layout.y + inputContext->lineRenderOffsetY;
    for (int i = inputContext->lineRenderIndexStart; i < inputContext->lineRenderIndexEnd; i++) {
        for (int j = 0; j < inputContext->buffer.arr[i].renderInfo.len; j++) {
            TextRenderInfo currInfo = inputContext->buffer.arr[i].renderInfo.arr[j];

            if (currInfo.text->num_lines == 0) {
                offsetY += inputContext->line_spacing;
                continue;
            }

            TextVertexUniform vertexUniform = (TextVertexUniform){
                .size = (float2){currInfo.textWidth, currInfo.textHeight},
                .position = (float2){
                    inputContext->layout.x - inputContext->scrollHorizontalOffset,
                    offsetY - inputContext->scrollVerticalOffset,
                },
                .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
                .pad = {0},
            };
            SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &vertexUniform, sizeof(vertexUniform));

            for (int k = 0; k < currInfo.sequence.len; k++) {
                TextRenderInfoSequence currSequence = currInfo.sequence.arr[k];

                TextFragmentUniform fragmentUniform = (TextFragmentUniform){
                    .color = (float4){
                        inputContext->textColor[0],
                        inputContext->textColor[1],
                        inputContext->textColor[2],
                        inputContext->textColor[3],
                    },
                    .imageType = currSequence.imageType,
                    .pad = {0},
                };
                SDL_PushGPUFragmentUniformData(renderContext->cmdbuf, 0, &fragmentUniform, sizeof(fragmentUniform));

                SDL_BindGPUVertexBuffers(
                    renderContext->renderPass,
                    0,
                    (SDL_GPUBufferBinding[]){{
                        .buffer = currSequence.vertexBuffer->buffer,
                        .offset = 0,
                    }},
                    1);

                SDL_BindGPUIndexBuffer(
                    renderContext->renderPass,
                    &(SDL_GPUBufferBinding){
                        .buffer = currSequence.indexBuffer->buffer,
                        .offset = 0,
                    },
                    SDL_GPU_INDEXELEMENTSIZE_32BIT);

                SDL_BindGPUFragmentSamplers(
                    renderContext->renderPass,
                    0,
                    (SDL_GPUTextureSamplerBinding[]){{
                        .sampler = renderContext->appContext->textSampler,
                        .texture = currSequence.atlas,
                    }},
                    1);

                SDL_DrawGPUIndexedPrimitives(renderContext->renderPass, currSequence.numIndices, 1, 0, 0, 0);
            }

            offsetY += inputContext->line_spacing;
        }
    }

    // Render highlights here
    int smallerHighlightLineIndex = SDL_min(inputContext->highlightStartLineIndex, inputContext->cursorLine);
    int highlightOffsetY = inputContext->layout.y;
    for (int i = 0; i < smallerHighlightLineIndex; i++) {
        highlightOffsetY += inputContext->line_spacing * inputContext->buffer.arr[i].renderInfo.len;
    }

    if (
        inputContext->highlightStartLineIndex == inputContext->cursorLine &&
        inputContext->highlightStartIndex != inputContext->cursorIndex) {
        int lineWidth = 0;
        int lineHeight = 0;
        int offsetX = 0;
        int tempY;
        if (inputContext->highlightStartIndex > inputContext->cursorIndex) {
            TTF_GetStringSize(
                inputContext->font,
                &inputContext->buffer.arr[inputContext->highlightStartLineIndex].arr[inputContext->cursorIndex],
                inputContext->highlightStartIndex - inputContext->cursorIndex,
                &lineWidth, &lineHeight);
            if (inputContext->cursorIndex > 0) {
                TTF_GetStringSize(
                    inputContext->font,
                    inputContext->buffer.arr[inputContext->highlightStartLineIndex].arr,
                    inputContext->cursorIndex,
                    &offsetX, &tempY);
            }
        } else if (inputContext->cursorIndex > inputContext->highlightStartIndex) {
            TTF_GetStringSize(
                inputContext->font,
                &inputContext->buffer.arr[inputContext->highlightStartLineIndex].arr[inputContext->highlightStartIndex],
                inputContext->cursorIndex - inputContext->highlightStartIndex,
                &lineWidth, &lineHeight);
            if (inputContext->highlightStartIndex > 0) {
                TTF_GetStringSize(
                    inputContext->font,
                    inputContext->buffer.arr[inputContext->highlightStartLineIndex].arr,
                    inputContext->highlightStartIndex,
                    &offsetX, &tempY);
            }
        }

        DrawRectangle(
            renderContext,
            (float[2]){
                inputContext->layout.x - inputContext->scrollHorizontalOffset + offsetX,
                highlightOffsetY - inputContext->scrollVerticalOffset,
            },
            (float[2]){lineWidth, inputContext->line_spacing},
            inputContext->highlightColor);
    }

    if (inputContext->highlightStartLineIndex > inputContext->cursorLine) {
        for (int i = inputContext->cursorLine; i <= inputContext->highlightStartLineIndex; i++) {
            int lineWidth = 0;
            int lineHeight = 0;
            int offsetX = 0;
            if (i == inputContext->cursorLine) {
                int tempY;
                TTF_GetStringSize(
                    inputContext->font,
                    &inputContext->buffer.arr[i].arr[inputContext->cursorIndex],
                    inputContext->buffer.arr[i].len - inputContext->cursorIndex,
                    &lineWidth, &lineHeight);
                if (inputContext->cursorIndex > 0) {
                    int tempY;
                    TTF_GetStringSize(
                        inputContext->font,
                        inputContext->buffer.arr[i].arr,
                        inputContext->cursorIndex,
                        &offsetX, &tempY);
                }

            } else if (i == inputContext->highlightStartLineIndex) {
                int tempY;
                if (inputContext->highlightStartIndex > 0) {
                    TTF_GetStringSize(
                        inputContext->font,
                        inputContext->buffer.arr[inputContext->highlightStartLineIndex].arr,
                        inputContext->highlightStartIndex,
                        &lineWidth, &lineHeight);
                }
            } else {
                if (inputContext->buffer.arr[i].len > 0) {
                    TTF_GetStringSize(inputContext->font, inputContext->buffer.arr[i].arr, inputContext->buffer.arr[i].len, &lineWidth, &lineHeight);
                } else {
                    // Empty lines will show highlights, and the width will be that of a space character
                    TTF_GetStringSize(inputContext->font, " ", 1, &lineWidth, &lineHeight);
                }
            }
            DrawRectangle(
                renderContext,
                (float[2]){
                    inputContext->layout.x - inputContext->scrollHorizontalOffset + offsetX,
                    highlightOffsetY - inputContext->scrollVerticalOffset,
                },
                (float[2]){lineWidth, inputContext->line_spacing},
                inputContext->highlightColor);
            highlightOffsetY += inputContext->line_spacing * inputContext->buffer.arr[i].renderInfo.len;
        }
    } else if (inputContext->highlightStartLineIndex < inputContext->cursorLine) {
        for (int i = inputContext->highlightStartLineIndex; i <= inputContext->cursorLine; i++) {
            int lineWidth = 0;
            int lineHeight = 0;
            int offsetX = 0;
            if (i == inputContext->highlightStartLineIndex) {
                int tempY;
                TTF_GetStringSize(
                    inputContext->font,
                    &inputContext->buffer.arr[i].arr[inputContext->highlightStartIndex],
                    inputContext->buffer.arr[i].len - inputContext->highlightStartIndex,
                    &lineWidth, &lineHeight);
                if (inputContext->highlightStartIndex > 0) {
                    int tempY;
                    TTF_GetStringSize(
                        inputContext->font,
                        inputContext->buffer.arr[i].arr,
                        inputContext->highlightStartIndex,
                        &offsetX, &tempY);
                }
            } else if (i == inputContext->cursorLine) {
                int tempY;
                if (inputContext->cursorIndex > 0) {
                    TTF_GetStringSize(
                        inputContext->font,
                        inputContext->buffer.arr[inputContext->cursorLine].arr,
                        inputContext->cursorIndex,
                        &lineWidth, &lineHeight);
                }
            } else {
                if (inputContext->buffer.arr[i].len > 0) {
                    TTF_GetStringSize(inputContext->font, inputContext->buffer.arr[i].arr, inputContext->buffer.arr[i].len, &lineWidth, &lineHeight);
                } else {
                    // Empty lines will show highlights, and the width will be that of a space character
                    TTF_GetStringSize(inputContext->font, " ", 1, &lineWidth, &lineHeight);
                }
            }
            DrawRectangle(
                renderContext,
                (float[2]){
                    inputContext->layout.x - inputContext->scrollHorizontalOffset + offsetX,
                    highlightOffsetY - inputContext->scrollVerticalOffset,
                },
                (float[2]){lineWidth, inputContext->line_spacing},
                inputContext->highlightColor);
            highlightOffsetY += inputContext->line_spacing * inputContext->buffer.arr[i].renderInfo.len;
        }
    }

    // Render the cursor here
    float cursor_width = 5.0f;

    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT_CURSOR]);

    TextCursorVertexUniform cursorVertexUniform = (TextCursorVertexUniform){
        .position = (float2){
            inputContext->cursorX + inputContext->layout.x - inputContext->scrollHorizontalOffset,
            inputContext->cursorY + inputContext->layout.y - inputContext->scrollVerticalOffset,
        },
        .size = (float2){cursor_width, (float)inputContext->cursorHeight},
        .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
        .pad = {0},
    };

    SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &cursorVertexUniform, sizeof(cursorVertexUniform));

    TextCursorFragmentUniform cursorFragmentUniform = (TextCursorFragmentUniform){
        .color = (float4){
            inputContext->cursorColor[0],
            inputContext->cursorColor[1],
            inputContext->cursorColor[2],
            inputContext->cursorColor[3],
        },
    };

    SDL_PushGPUFragmentUniformData(renderContext->cmdbuf, 0, &cursorFragmentUniform, sizeof(cursorFragmentUniform));

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);

    if (scissorStateWasUpdated == true) {
        // Make sure the scissor goes back to the size of the render target
        SDL_SetGPUScissor(
            renderContext->renderPass,
            &(SDL_Rect){
                .x = 0,
                .y = 0,
                .w = renderContext->renderWidth,
                .h = renderContext->renderHeight,
            });
    }
}