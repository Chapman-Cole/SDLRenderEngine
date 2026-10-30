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
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT]);

    float offsetY = inputContext->layout.y;
    for (int i = 0; i < inputContext->buffer.len; i++) {
        TextRenderInfo currInfo = inputContext->buffer.arr[i].renderInfo;

        if (currInfo.text->num_lines == 0) {
            offsetY += inputContext->line_spacing;
            continue;
        }

        TextVertexUniform vertexUniform = (TextVertexUniform){
            .size = (float2){currInfo.textWidth, currInfo.textHeight},
            .position = (float2){inputContext->layout.x, offsetY},
            .windowDims = (float2){renderContext->renderWidth, renderContext->renderHeight},
            .pad = {0},
        };
        SDL_PushGPUVertexUniformData(renderContext->cmdbuf, 0, &vertexUniform, sizeof(vertexUniform));

        for (int j = 0; j < currInfo.sequence.len; j++) {
            TextRenderInfoSequence currSequence = currInfo.sequence.arr[j];

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

    // Render the cursor here

    float cursor_width = 2.0f;

    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT_CURSOR]);

    TextCursorVertexUniform cursorVertexUniform = (TextCursorVertexUniform){
        .position = (float2){
            (float)inputContext->cursorX + inputContext->layout.x,
            (float)inputContext->cursorY + inputContext->layout.y,
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
}