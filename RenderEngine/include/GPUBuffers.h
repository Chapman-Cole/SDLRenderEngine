#ifndef GPUBUFFERS_H
#define GPUBUFFERS_H

#include "AppContext.h"

typedef struct TransferContextInfo {
    SDL_GPUTransferBuffer* transferBuffer;
    SDL_GPUBuffer* gpuBuffer;
    Uint32 bufferSize;
} TransferContextInfo;

typedef struct TransferContextInfoArray {
    TransferContextInfo* arr;
    size_t len;
    size_t capacity;
} TransferContextInfoArray;

typedef struct GPUTransferContext {
    TransferContextInfoArray contextInfo;
    SDL_GPUCommandBuffer* cmdbuf;
    SDL_GPUCopyPass* copyPass;
    AppContext* appContext;
} GPUTransferContext;

int GPUTransferContextBegin(GPUTransferContext* transferContext, AppContext* appContext);

int GPUTransferContextEnd(GPUTransferContext* transferContext);

// Called in between GPUTransferContextBegin and GPUTransferContextEnd
// data_size should be the total size in bytes
int GPUTransferContextUpload(GPUTransferContext* transferContext, SDL_GPUBuffer* gpuBuffer, void* data, Uint32 data_size);

#endif
