#ifndef GPUTRANSFERCONTEXT_H
#define GPUTRANSFERCONTEXT_H

#include "MemoryArena.h"
#include "SDL3/SDL.h"
#include "GPUBufferContext.h"

typedef struct TransferContextInfo {
    GPUBuffer* gpuBuffer;
    void* data;
    Uint32 dataSize;
    Uint32 tbIndex;
    Uint32 tbOffset;
} TransferContextInfo;

typedef struct TransferContextInfoArray {
    TransferContextInfo* arr;
    size_t len;
    size_t capacity;
} TransferContextInfoArray;

typedef struct TransferBufferInfo {
    SDL_GPUTransferBuffer* transferBuffer;
    Uint32 bufferSize;
    Uint32 amountFilled;
} TransferBufferInfo;

typedef struct TransferBufferInfoArray {
    TransferBufferInfo* arr;
    size_t len;
    size_t capacity;
} TransferBufferInfoArray;

typedef struct GPUTransferContext {
    TransferContextInfoArray contextInfo;
    TransferBufferInfoArray transferBuffers;
    MemoryArena dataStagingArena;
    SDL_GPUCommandBuffer* cmdbuf;
    SDL_GPUCopyPass* copyPass;
    SDL_GPUDevice* device;
} GPUTransferContext;

int GPUTransferContextCreate(GPUTransferContext* transferContext, SDL_GPUDevice* device, size_t arenaSize);

int GPUTransferContextDestroy(GPUTransferContext* transferContext);

int GPUTransferContextSubmit(GPUTransferContext* transferContext);

// data_size should be the total size in bytes
int GPUTransferContextUpload(GPUTransferContext* transferContext, GPUBuffer* gpuBuffer, void* data, Uint32 data_size);

#endif
