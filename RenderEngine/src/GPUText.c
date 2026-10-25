#include "GPUText.h"
#include "GPUTransferContext.h"

typedef SDL_FPoint Vec2;

typedef struct TextVertex {
    Vec2 point;
    Vec2 tex;
} TextVertex;

static inline int generateGPUBuffers(GPUText* renderText, AppContext* appContext) {
    TTF_GPUAtlasDrawSequence* sequence = TTF_GetGPUTextDrawData(renderText->text);
    if (sequence == NULL) {
        // NULL sequence means there is nothing to render
        renderText->numAtlasses = 0;
        renderText->atlasses = NULL;
        return 0;
    }

    Uint32 numAtlasses = 0;

    TTF_GPUAtlasDrawSequence* tempSequence = sequence;
    for (; sequence != NULL; sequence = sequence->next) {
        numAtlasses++;
    }
    sequence = tempSequence;

    renderText->numAtlasses = numAtlasses;
    renderText->atlasses = SDL_calloc(numAtlasses, sizeof(TextSequence));

    numAtlasses = 0;
    for (; sequence != NULL; sequence = sequence->next) {
        TransferContextData vertexData = GPUTransferContextAcquireStagingBuffer(&appContext->transferContext, sequence->num_vertices * sizeof(TextVertex));
        TransferContextData fragmentData = GPUTransferContextAcquireStagingBuffer(&appContext->transferContext, sequence->num_indices * sizeof(int));

        TextVertex* vertices = vertexData.buffer;
        int* indices = fragmentData.buffer;

        for (int i = 0; i < sequence->num_vertices; i++) {
            vertices[i] = (TextVertex){.point = sequence->xy[i], .tex = sequence->uv[i]};
        }

        SDL_memcpy(indices, sequence->indices, sequence->num_indices * sizeof(int));

        TextSequence textSequence = {0};
        textSequence.numIndices = sequence->num_indices;
        textSequence.atlas = sequence->atlas_texture;

        textSequence.vertexBuffer = GPUBuffer_create(appContext->device, &(SDL_GPUBufferCreateInfo){
                                                                             .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                                                                             .size = sequence->num_vertices * sizeof(TextVertex),
                                                                             .props = 0,
                                                                         });

        textSequence.indexBuffer = GPUBuffer_create(appContext->device, &(SDL_GPUBufferCreateInfo){
                                                                            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                                                                            .size = sequence->num_indices * sizeof(int),
                                                                            .props = 0,
                                                                        });

        GPUTransferContextUpload(&appContext->transferContext, textSequence.vertexBuffer, vertexData);
        GPUTransferContextUpload(&appContext->transferContext, textSequence.indexBuffer, fragmentData);

        renderText->atlasses[numAtlasses] = textSequence;

        numAtlasses++;
    }

    return 0;
}

int GPUTextCreate(GPUText* renderText, AppContext* appContext, TTF_Font* font, const char* text, uint32_t length, float color[4]) {
    renderText->appContext = appContext;
    renderText->font = font;

    renderText->color[0] = color[0];
    renderText->color[1] = color[1];
    renderText->color[2] = color[2];
    renderText->color[3] = color[3];

    renderText->text = TTF_CreateText(appContext->textEngine, font, text, length);
    if (renderText->text == NULL) {
        SDL_Log("Failed to create text: %s", SDL_GetError());
        return -1;
    }

    if (!TTF_GetTextSize(renderText->text, &renderText->width, &renderText->height)) {
        SDL_Log("Failed to get text size: %s", SDL_GetError());
        return -1;
    }

    if (!TTF_GetTextPosition(renderText->text, &renderText->x, &renderText->y)) {
        SDL_Log("Failed to get text position: %s", SDL_GetError());
        return -1;
    }

    if (generateGPUBuffers(renderText, appContext) < 0) {
        return -1;
    }

    return 0;
}

int GPUTextDestroy(GPUText* renderText) {
    for (Uint32 i = 0; i < renderText->numAtlasses; i++) {
        GPUBuffer_unref(renderText->atlasses[i].vertexBuffer);
        GPUBuffer_unref(renderText->atlasses[i].indexBuffer);
    }
    SDL_free(renderText->atlasses);
    TTF_DestroyText(renderText->text);
    return 0;
}

int GPUTextUpdate(GPUText* renderText, const char* text, uint32_t length) {
    for (Uint32 i = 0; i < renderText->numAtlasses; i++) {
        GPUBuffer_unref(renderText->atlasses[i].vertexBuffer);
        GPUBuffer_unref(renderText->atlasses[i].indexBuffer);
    }
    SDL_free(renderText->atlasses);

    TTF_SetTextString(renderText->text, text, length);

    TTF_GetTextSize(renderText->text, &renderText->width, &renderText->height);
    TTF_GetTextPosition(renderText->text, &renderText->x, &renderText->y);

    if (generateGPUBuffers(renderText, renderText->appContext) < 0) {
        return -1;
    }

    return 0;
}

int GPUTextUpdateFont(GPUText* renderText, TTF_Font* font) {
    renderText->font = font;
    TTF_SetTextFont(renderText->text, renderText->font);
    if (generateGPUBuffers(renderText, renderText->appContext) < 0) {
        return -1;
    }
    return 0;
}