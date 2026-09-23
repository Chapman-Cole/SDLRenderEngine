#include "AppContext.h"
#include "DrawFunctions.h"
#include "GPUBuffers.h"
#include "RenderContext.h"
#include "Shaders.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static float vertices[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f};

static Uint16 indices[] = {
    0, 1, 2,
    1, 3, 2};

int main(int argc, char* argv[]) {
    AppContext appContext = {0};
    appContext.windowName = "Renderer";
    appContext.deviceName = NULL;
    AppContextInit(&appContext, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_SetGPUSwapchainParameters(appContext.device, appContext.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

    SDL_GPUGraphicsPipeline* graphicsPipeline = GraphicsPipelineCreate(
        &appContext,
        ShaderLoad(&appContext, "clear.vert.spv", 0, 0, 0, 0),
        ShaderLoad(&appContext, "clear.frag.spv", 0, 0, 0, 0));

    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(appContext.device, &(SDL_GPUBufferCreateInfo){
                                                                             .size = sizeof(vertices),
                                                                             .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                                                             .props = 0});

    SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(appContext.device, &(SDL_GPUBufferCreateInfo){
                                                                            .size = sizeof(indices),
                                                                            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                                                            .props = 0});

    GPUTransferContext transferContext = {0};
    GPUTransferContextBegin(&transferContext, &appContext);
    GPUTransferContextUpload(&transferContext, vertexBuffer, vertices, sizeof(vertices));
    GPUTransferContextUpload(&transferContext, indexBuffer, indices, sizeof(indices));
    GPUTransferContextEnd(&transferContext);

    bool shouldContinue = true;
    while (shouldContinue) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                shouldContinue = false;
                continue;
            }
        }

        RenderContext renderContext = {0};
        renderContext.texture = NULL;
        RenderContextBegin(&renderContext, &appContext, NULL);

        // SDL_BindGPUGraphicsPipeline(renderContext.renderPass, graphicsPipeline);
        // SDL_BindGPUVertexBuffers(renderContext.renderPass, 0, &(SDL_GPUBufferBinding){.buffer = vertexBuffer, .offset = 0}, 1);
        // SDL_BindGPUIndexBuffer(
        //     renderContext.renderPass,
        //     &(SDL_GPUBufferBinding){
        //         .buffer = indexBuffer,
        //         .offset = 0,
        //     },
        //     SDL_GPU_INDEXELEMENTSIZE_16BIT);
        // SDL_DrawGPUIndexedPrimitives(renderContext.renderPass, 6, 1, 0, 0, 0);

        const float currTime = (float)SDL_GetTicks() / 1000.0f;

        // DrawRectangle(&renderContext, (float[]){0.5 * SDL_cos(currTime), 0.5 * SDL_sin(currTime)}, (float[]){0.3, 0.3}, (float[]){0.4, 0.1, 0.6, 1.0});
        // DrawRectangle(&renderContext, (float[]){0.5 * SDL_cos(currTime + SDL_PI_F / 2), 0.5 * SDL_sin(currTime + SDL_PI_F / 2)}, (float[]){0.3, 0.3}, (float[]){0.2, 0.6, 0.1, 1.0});
        // DrawRectangle(&renderContext, (float[]){0.5 * SDL_cos(currTime + SDL_PI_F), 0.5 * SDL_sin(currTime + SDL_PI_F)}, (float[]){0.3, 0.3}, (float[]){0.7, 0.3, 0.6, 1.0});
        // DrawRectangle(&renderContext, (float[]){0.5 * SDL_cos(currTime + SDL_PI_F * 1.5), 0.5 * SDL_sin(currTime + SDL_PI_F * 1.5)}, (float[]){0.3, 0.3}, (float[]){0.3, 0.5, 0.4, 1.0});

        // DrawRectangleRounded(&renderContext, (float[]){-0.75, -0.75}, (float[]){0.5, 0.5}, (float[]){0.3, 0.4, 0.2, 1.0}, 0.3);
        // DrawRectangleRounded(&renderContext, (float[]){0, 0}, (float[]){1, 1}, (float[]){0.1, 0.5, 0.3, 1.0}, 0.2);

        const int numCircles = 12;
        for (int i = 0; i < numCircles; i++) {
            const float currAngle = i * 2.0f * SDL_PI_F / (float)numCircles;

            DrawRectangleRounded(
                &renderContext,
                (float[]){0.6f * SDL_cosf(currAngle) * renderContext.aspectRatio, 0.6f * SDL_sinf(currAngle)},
                (float[]){0.23, 0.23},
                (float[]){0.2, 0.4, 0.2, 1.0},
                0.7);

            DrawCircle(
                &renderContext,
                (float[]){0.6f * SDL_cosf(currAngle) * renderContext.aspectRatio, 0.6f * SDL_sinf(currAngle)},
                0.1,
                (float[]){0.3, 0.6, 0.2, 1.0});
        }

        RenderContextEnd(&renderContext);
    }

    SDL_ReleaseGPUBuffer(appContext.device, vertexBuffer);
    SDL_ReleaseGPUBuffer(appContext.device, indexBuffer);
    SDL_ReleaseGPUGraphicsPipeline(appContext.device, graphicsPipeline);
    AppContextTerminate(&appContext);
    return 0;
}