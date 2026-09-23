#ifndef GPUBUFFERS_H
#define GPUBUFFERS_H

#include "SDL3/SDL.h"
#include "AppContext.h"

#define GPUBUFFERS_MAX_UPLOAD 20

typedef struct GPUTransferContext {
    SDL_GPUCommandBuffer* cmdbuf;
    SDL_GPUCopyPass* copyPass;
    SDL_GPUTransferBuffer* transferBuffers[GPUBUFFERS_MAX_UPLOAD];
    SDL_GPUBuffer* gpuBuffers[GPUBUFFERS_MAX_UPLOAD];
    Uint32 bufferSizes[GPUBUFFERS_MAX_UPLOAD];
    AppContext* appContext;
    Uint32 bufferLen;
} GPUTransferContext;

int GPUTransferContextBegin(GPUTransferContext* transferContext, AppContext* appContext);

int GPUTransferContextEnd(GPUTransferContext* transferContext);

// Called in between GPUTransferContextBegin and GPUTransferContextEnd
// data_size should be the total size in bytes
int GPUTransferContextUpload(GPUTransferContext* transferContext, SDL_GPUBuffer* gpuBuffer, void* data, Uint32 data_size);

#endif