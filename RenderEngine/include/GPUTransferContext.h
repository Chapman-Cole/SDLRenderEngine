#ifndef GPUTRANSFERCONTEXT_H
#define GPUTRANSFERCONTEXT_H

#include "MemoryArena.h"
#include "SDL3/SDL.h"
#include "GPUBufferContext.h"

typedef struct TransferContextData {
    void* buffer;
    Uint32 buffer_size;
    bool _stagingBuffer;
} TransferContextData;

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

#define TRANSFER_CONTEXT_DATA(data, data_size) (TransferContextData){.buffer = data, .buffer_size = data_size, ._stagingBuffer = false}

int GPUTransferContextCreate(GPUTransferContext* transferContext, SDL_GPUDevice* device, size_t arenaSize);

int GPUTransferContextDestroy(GPUTransferContext* transferContext);

int GPUTransferContextSubmit(GPUTransferContext* transferContext);

// If the data planned on being uploaded will be destroyed after calling GPUTransferContextUpload and you know the size of
// the buffer aheaad of time, you can avoid unnecessary copying of data by acquiring a staging buffer to directly upload your
// data to instead. Do not try freeing the pointer in this buffer.
TransferContextData GPUTransferContextAcquireStagingBuffer(GPUTransferContext* transferContext, Uint32 data_size);

// TransferContextData not acquired from GPUTransferContextAcquireStagingBuffer should be passed via the TRANSFER_CONTEXT_DATA
// macro with the pointer (data) and size (data_size) respectively
int GPUTransferContextUpload(GPUTransferContext* transferContext, GPUBuffer* gpuBuffer, TransferContextData data);

#endif
