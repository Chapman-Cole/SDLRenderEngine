#ifndef GPUBUFFERCONTEXT_H
#define GPUBUFFERCONTEXT_H

#include "SDL3/SDL.h"

typedef struct GPUBuffer {
    SDL_GPUBuffer* buffer;
    Uint32 refCount;
} GPUBuffer;

#define GPUBUFFER_ACQUIRE_REFERENCE(gpuBuffer) ((gpuBuffer)->refCount++);

static inline GPUBuffer* GPUBuffer_create(SDL_GPUDevice* device, const SDL_GPUBufferCreateInfo* createInfo) {
    GPUBuffer* gpu_buffer = (GPUBuffer*)SDL_malloc(sizeof(GPUBuffer));
    if (gpu_buffer == NULL) {
        SDL_Log("Failed to allocate memory for gpu buffer: %s", SDL_GetError());
        return NULL;
    }

    gpu_buffer->buffer = SDL_CreateGPUBuffer(device, createInfo);
    if (gpu_buffer->buffer == NULL) {
        SDL_Log("Failed to create gpu buffer: %s", SDL_GetError());
        return NULL;
    }

    gpu_buffer->refCount = 1;

    return gpu_buffer;
}

static inline void GPUBuffer_unref(SDL_GPUDevice* device, GPUBuffer* gpuBuffer) {
    gpuBuffer->refCount--;
    if (gpuBuffer->refCount == 0) {
        SDL_ReleaseGPUBuffer(device, gpuBuffer->buffer);
        SDL_free(gpuBuffer);
    }
}


#endif