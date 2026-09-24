#include "DrawFunctions.h"

void DrawRectangle(RenderContext* renderContext, float center[2], float size[2], float color[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[32]){center[0], center[1], size[0], size[1], renderContext->aspectRatio, 0, 0, 0},
        32);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[48]){
            color[0], color[1], color[2], color[3], // color
            center[0], center[1],                   // center
            size[0], size[1],                       // size
            0,                                      // roundness (should be 0 since this is a normal rectangle)
            renderContext->aspectRatio,             // aspect ratio
            0, 0                                    // padding
        },
        48);

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawRectangleRounded(RenderContext* renderContext, float center[2], float size[2], float color[4], float roundness) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[32]){center[0], center[1], size[0], size[1], renderContext->aspectRatio, 0, 0, 0},
        32);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[48]){
            color[0], color[1], color[2], color[3], // color
            center[0], center[1],                   // center
            size[0], size[1],                       // size
            roundness,                              // roundness (should be 0 since this is a normal rectangle)
            renderContext->aspectRatio,             // aspect ratio
            0, 0                                    // padding
        },
        48);

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawCircle(RenderContext* renderContext, float center[2], float radius, float color[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_CIRCLE]);

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[16]){center[0], center[1], radius, renderContext->aspectRatio},
        16);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[32]){
            color[0], color[1], color[2], color[3], // color
            center[0], center[1],                   // center
            radius,                                 // radius
            renderContext->aspectRatio              // aspect ratio
        },
        32);

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawText(RenderContext* renderContext, GPUText* text) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT]
    );

    SDL_BindGPUVertexBuffers(
        renderContext->renderPass,
        0,
        (SDL_GPUBufferBinding[]){{
            .buffer = text->vertexBuffer,
            .offset = 0,
        }},
        1
    );

    SDL_BindGPUIndexBuffer(
        renderContext->renderPass,
        &(SDL_GPUBufferBinding){
            .buffer = text->indexBuffer,
            .offset = 0
        },
        SDL_GPU_INDEXELEMENTSIZE_32BIT
    );

    SDL_DrawGPUIndexedPrimitives(renderContext->renderPass, text->numIndices, 1, 0, 0, 0);
}