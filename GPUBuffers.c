#include "GPUBuffers.h"

int GPUTransferContextBegin(GPUTransferContext* transferContext, AppContext* appContext) {
    transferContext->bufferLen = 0;
    transferContext->appContext = appContext;
    return 0;
}

int GPUTransferContextEnd(GPUTransferContext* transferContext) {
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(transferContext->appContext->device);
    SDL_GPUCopyPass* copypass = SDL_BeginGPUCopyPass(cmdbuf);

    for (Uint32 i = 0; i < transferContext->bufferLen; i++) {
        SDL_UploadToGPUBuffer(
            copypass,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = transferContext->transferBuffers[i],
                .offset = 0
            },
            &(SDL_GPUBufferRegion){
                .buffer = transferContext->gpuBuffers[i],
                .offset = 0,
                .size = transferContext->bufferSizes[i]
            },
            false
        );
    }

    SDL_EndGPUCopyPass(copypass);
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    for (Uint32 i = 0; i < transferContext->bufferLen; i++) {
        SDL_ReleaseGPUTransferBuffer(transferContext->appContext->device, transferContext->transferBuffers[i]);
    }

    return 0;
}

int GPUTransferContextUpload(GPUTransferContext* transferContext, SDL_GPUBuffer* gpuBuffer, void* data, Uint32 data_size) {
    Uint32 tbLen = transferContext->bufferLen;
    transferContext->transferBuffers[tbLen] = SDL_CreateGPUTransferBuffer(
        transferContext->appContext->device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = data_size});

    if (transferContext->transferBuffers == NULL) {
        SDL_Log("Failed to create transfer buffer.\n");
        return -1;
    }

    void* transferData = SDL_MapGPUTransferBuffer(
        transferContext->appContext->device,
        transferContext->transferBuffers[tbLen],
        false
    );

    SDL_memcpy(transferData, data, data_size);

    SDL_UnmapGPUTransferBuffer(
        transferContext->appContext->device,
        transferContext->transferBuffers[tbLen]
    );

    transferContext->gpuBuffers[tbLen] = gpuBuffer;
    transferContext->bufferSizes[tbLen] = data_size;

    transferContext->bufferLen++;

    if (transferContext->bufferLen >= GPUBUFFERS_MAX_UPLOAD) {
        SDL_Log("Exceeded maximum number of buffers allowed in a GPUTransferContext: %d BUFFER LIMIT", GPUBUFFERS_MAX_UPLOAD);
        return -1;
    }

    return 0;
}