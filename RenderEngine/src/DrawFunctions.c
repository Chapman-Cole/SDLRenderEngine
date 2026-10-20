#include "DrawFunctions.h"

void DrawRectangle(RenderContext* renderContext, float position[2], float size[2], float color[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[8]){
            position[0],
            position[1],
            size[0],
            size[1],
            renderContext->renderWidth,
            renderContext->renderHeight,
            0,
            0,
        },
        32);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[12]){
            color[0],
            color[1],
            color[2],
            color[3], // color
            position[0],
            position[1], // center
            size[0],
            size[1], // size
            0,       // roundness (should be 0 since this is a normal rectangle)
            0,
            0,
            0, // padding
        },
        48);

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

void DrawRectangleRounded(RenderContext* renderContext, float position[2], float size[2], float color[4], float cornerRadii[4]) {
    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[8]){
            position[0],
            position[1],
            size[0],
            size[1],
            renderContext->renderWidth,
            renderContext->renderHeight,
            0,
            0,
        },
        32);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[12]){
            color[0],
            color[1],
            color[2],
            color[3], // color
            position[0],
            position[1], // center
            size[0],
            size[1], // size
            cornerRadii[0],
            cornerRadii[1],
            cornerRadii[2],
            cornerRadii[3], // cornerRadii
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
        (float[32]){
            center[0],
            center[1],
            renderContext->renderWidth,
            renderContext->renderHeight,
            radius,
            renderContext->aspectRatio,
        },
        32);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[8]){
            color[0], color[1], color[2], color[3], // color
            center[0], center[1],                   // center
            radius,                                 // radius
            renderContext->aspectRatio              // aspect ratio
        },
        32);

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

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[12]){
            (float)text->width,
            (float)text->height, // text size
            (float)text->x,
            (float)text->y, // text internal position
            renderContext->renderWidth,
            renderContext->renderHeight,             // Window Dimensions
            renderContext->aspectRatio,              // aspect ratio
            renderContext->appContext->displayScale, // display scale for high dpi screens
            position[0],
            position[1], // Text offset/position
            0,
            0, // padding
        },
        48);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[4]){
            text->color[0],
            text->color[1],
            text->color[2],
            text->color[3],
        },
        16);

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
                .offset = 0},
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

/*
void DrawInputContext(RenderContext* renderContext, InputContext* inputContext) {
    DrawText(renderContext, &inputContext->gpuText, (float[2]){inputContext->layout.x, inputContext->layout.y});

    // Render the cursor here

    SDL_BindGPUGraphicsPipeline(
        renderContext->renderPass,
        renderContext->appContext->builtinPipelines[BUILTIN_PIPELINE_TEXT_CURSOR]);

    SDL_PushGPUVertexUniformData(
        renderContext->cmdbuf,
        0,
        (float[8]){
            (float)inputContext->cursorX + inputContext->layout.x,
            (float)inputContext->cursorY + inputContext->layout.y, // Cursor Position
            2.0f,
            (float)inputContext->cursorHeight, // Cursor Size
            renderContext->renderWidth,
            renderContext->renderHeight, // Window Dimensions
            0,
            0, // Padding
        },
        32);

    SDL_PushGPUFragmentUniformData(
        renderContext->cmdbuf,
        0,
        (float[4]){
            inputContext->cursorColor[0],
            inputContext->cursorColor[1],
            inputContext->cursorColor[2],
            inputContext->cursorColor[3],
        },
        16);

    SDL_DrawGPUPrimitives(renderContext->renderPass, 6, 1, 0, 0);
}

*/