#include "GPUBuffers.h"
#include "DynamicArray.h"

int GPUTransferContextBegin(GPUTransferContext* transferContext, AppContext* appContext) {
    DYNAMIC_ARRAY_INIT(transferContext->contextInfo);
    transferContext->appContext = appContext;
    return 0;
}

int GPUTransferContextEnd(GPUTransferContext* transferContext) {
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(transferContext->appContext->device);
    SDL_GPUCopyPass* copypass = SDL_BeginGPUCopyPass(cmdbuf);

    for (Uint32 i = 0; i < transferContext->contextInfo.len; i++) {
        SDL_UploadToGPUBuffer(
            copypass,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = transferContext->contextInfo.arr[i].transferBuffer,
                .offset = 0
            },
            &(SDL_GPUBufferRegion){
                .buffer = transferContext->contextInfo.arr[i].gpuBuffer,
                .offset = 0,
                .size = transferContext->contextInfo.arr[i].bufferSize
            },
            false
        );
    }

    SDL_EndGPUCopyPass(copypass);
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    for (Uint32 i = 0; i < transferContext->contextInfo.len; i++) {
        SDL_ReleaseGPUTransferBuffer(transferContext->appContext->device, transferContext->contextInfo.arr[i].transferBuffer);
    }

    DYNAMIC_ARRAY_FREE(transferContext->contextInfo);
    return 0;
}

int GPUTransferContextUpload(GPUTransferContext* transferContext, SDL_GPUBuffer* gpuBuffer, void* data, Uint32 data_size) {
    TransferContextInfo transferContextInfo = {0};
    transferContextInfo.transferBuffer = SDL_CreateGPUTransferBuffer(
        transferContext->appContext->device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = data_size});

    if (transferContextInfo.transferBuffer == NULL) {
        SDL_Log("Failed to create transfer buffer.\n");
        return -1;
    }

    void* transferData = SDL_MapGPUTransferBuffer(
        transferContext->appContext->device,
        transferContextInfo.transferBuffer,
        false
    );

    SDL_memcpy(transferData, data, data_size);

    SDL_UnmapGPUTransferBuffer(
        transferContext->appContext->device,
        transferContextInfo.transferBuffer
    );

    transferContextInfo.gpuBuffer = gpuBuffer;
    transferContextInfo.bufferSize = data_size;

    DYNAMIC_ARRAY_APPEND(transferContext->contextInfo, transferContextInfo);

    return 0;
}