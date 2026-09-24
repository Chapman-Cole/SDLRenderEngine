#include "GPUText.h"
#include "GPUBuffers.h"

typedef SDL_FPoint Vec2;

int GPUTextCreate(GPUText* renderText, AppContext* appContext, TTF_Font* font, const char* text) {
    renderText->appContext = appContext;

    renderText->text = TTF_CreateText(appContext->textEngine, font, text, 0);
    if (renderText->text == NULL) {
        SDL_Log("Failed to create text: %s", SDL_GetError());
        return -1;
    }

    TTF_GPUAtlasDrawSequence* sequence = TTF_GetGPUTextDrawData(renderText->text);
    if (sequence == NULL) {
        SDL_Log("Failed to create TTF_GPUAtlasDrawSequence: %s", SDL_GetError());
        return -1;
    }

    Uint32 numVertices = 0;
    Uint32 numIndices = 0;

    TTF_GPUAtlasDrawSequence* tempSequence = sequence;
    for (; sequence != NULL; sequence = sequence->next) {
        numVertices += sequence->num_vertices;
        numIndices += sequence->num_indices;
    }
    sequence = tempSequence;

    Vec2* vertices = SDL_calloc(numVertices, sizeof(Vec2));
    int* indices = SDL_calloc(numIndices, sizeof(int));

    numVertices = 0;
    numIndices = 0;
    for (; sequence != NULL; sequence = sequence->next) {
        SDL_memcpy(vertices + numVertices, sequence->xy, sequence->num_vertices * sizeof(SDL_FPoint));
        SDL_memcpy(indices + numIndices, sequence->indices, sequence->num_indices * sizeof(int));
        numVertices += sequence->num_vertices;
        numIndices += sequence->num_indices;
    }

    renderText->numIndices = numIndices;

    renderText->vertexBuffer = SDL_CreateGPUBuffer(
        appContext->device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = numVertices * sizeof(Vec2),
            .props = 0});

    if (renderText->vertexBuffer == NULL) {
        SDL_Log("Failed to create a vertex buffer for text object: %s", SDL_GetError());
        SDL_free(vertices);
        SDL_free(indices);
        return -1;
    }

    renderText->indexBuffer = SDL_CreateGPUBuffer(
        appContext->device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = numIndices * sizeof(int),
            .props = 0});

    if (renderText->indexBuffer == NULL) {
        SDL_Log("Failed to create index buffer for text object: %s", SDL_GetError());
        SDL_free(vertices);
        SDL_free(indices);
        return -1;
    }

    GPUTransferContext transferContext = {0};
    GPUTransferContextBegin(&transferContext, appContext);
    GPUTransferContextUpload(&transferContext, renderText->vertexBuffer, vertices, numVertices * sizeof(Vec2));
    GPUTransferContextUpload(&transferContext, renderText->indexBuffer, indices, numIndices * sizeof(int));
    GPUTransferContextEnd(&transferContext);

    SDL_free(vertices);
    SDL_free(indices);

    return 0;
}

int GPUTextDestroy(GPUText* renderText) {
    SDL_ReleaseGPUBuffer(renderText->appContext->device, renderText->vertexBuffer);
    SDL_ReleaseGPUBuffer(renderText->appContext->device, renderText->indexBuffer);
    TTF_DestroyText(renderText->text);
    return 0;
}