#include "GPUTransferContext.h"
#include "DynamicArray.h"
#include <stddef.h>

// It may be ideal to change this size later on, but for now
// a megabyte should be a good transfer buffer size for each of them
#define TRANSFER_BUFFER_SIZE 1'000'000

int GPUTransferContextCreate(GPUTransferContext* transferContext, SDL_GPUDevice* device, size_t arenaSize) {
    MemoryArena_create(&transferContext->dataStagingArena, arenaSize);

    DYNAMIC_ARRAY_INIT(transferContext->transferBuffers);

    TransferBufferInfo tbInfo = {0};
    tbInfo.bufferSize = TRANSFER_BUFFER_SIZE;
    tbInfo.amountFilled = 0;
    tbInfo.transferBuffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = tbInfo.bufferSize,
            .props = 0,
        });

    if (tbInfo.transferBuffer == NULL) {
        SDL_Log("Failed to allocate memory for transfer buffer: %s", SDL_GetError());
        return -1;
    }

    DYNAMIC_ARRAY_APPEND(transferContext->transferBuffers, tbInfo);

    DYNAMIC_ARRAY_INIT(transferContext->contextInfo);
    transferContext->device = device;
    return 0;
}

int GPUTransferContextDestroy(GPUTransferContext* transferContext) {
    MemoryArena_destroy(&transferContext->dataStagingArena);

    for (int i = 0; i < transferContext->transferBuffers.len; i++) {
        SDL_ReleaseGPUTransferBuffer(transferContext->device, transferContext->transferBuffers.arr[i].transferBuffer);
    }

    DYNAMIC_ARRAY_FREE(transferContext->transferBuffers);
    DYNAMIC_ARRAY_FREE(transferContext->contextInfo);
    return 0;
}

int GPUTransferContextSubmit(GPUTransferContext* transferContext) {
    if (transferContext->contextInfo.len == 0) {
        return 0;
    }

    Uint32 currTransferBufferIndex = transferContext->contextInfo.arr[0].tbIndex;
    SDL_GPUTransferBuffer* transferBuffer = transferContext->transferBuffers.arr[currTransferBufferIndex].transferBuffer;
    void* transfer_data = SDL_MapGPUTransferBuffer(
        transferContext->device,
        transferBuffer,
        true // cycling allows reuse of the same transfer buffers even if the gpu is still using that transfer buffer for something else
    );

    for (Uint32 i = 0; i < transferContext->contextInfo.len; i++) {
        TransferContextInfo currInfo = transferContext->contextInfo.arr[i];

        if (currInfo.tbIndex != currTransferBufferIndex) {
            currTransferBufferIndex = currInfo.tbIndex;
            SDL_UnmapGPUTransferBuffer(
                transferContext->device,
                transferBuffer);

            transferBuffer = transferContext->transferBuffers.arr[currTransferBufferIndex].transferBuffer;

            transfer_data = SDL_MapGPUTransferBuffer(
                transferContext->device,
                transferBuffer,
                true // cycling allows reuse of the same transfer buffers even if the gpu is still using that transfer buffer for something else
            );
        }

        SDL_memcpy((uint8_t*)transfer_data + currInfo.tbOffset, currInfo.data, currInfo.dataSize);
    }

    SDL_UnmapGPUTransferBuffer(
        transferContext->device,
        transferBuffer);

    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(transferContext->device);
    if (cmdbuf == NULL) {
        SDL_Log("Failed to acquire command buffer transfer context upload: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUCopyPass* copypass = SDL_BeginGPUCopyPass(cmdbuf);

    for (Uint32 i = 0; i < transferContext->contextInfo.len; i++) {
        TransferContextInfo currInfo = transferContext->contextInfo.arr[i];
        SDL_GPUTransferBuffer* transferBuffer = transferContext->transferBuffers.arr[currInfo.tbIndex].transferBuffer;

        SDL_UploadToGPUBuffer(
            copypass,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = transferBuffer,
                .offset = currInfo.tbOffset,
            },
            &(SDL_GPUBufferRegion){
                .buffer = currInfo.gpuBuffer->buffer,
                .offset = 0,
                .size = currInfo.dataSize,
            },
            true // enable cycling so that in case the gpu buffer is actively bound
        );
    }

    SDL_EndGPUCopyPass(copypass);
    SDL_SubmitGPUCommandBuffer(cmdbuf);

    for (int i = 0; i < transferContext->transferBuffers.len; i++) {
        transferContext->transferBuffers.arr[i].amountFilled = 0;
    }

    for (int i = 0; i < transferContext->contextInfo.len; i++) {
        GPUBuffer_unref(transferContext->device, transferContext->contextInfo.arr[i].gpuBuffer);
    }

    DYNAMIC_ARRAY_CLEAR(transferContext->contextInfo);
    MemoryArena_clear(&transferContext->dataStagingArena);

    return 0;
}

int GPUTransferContextUpload(GPUTransferContext* transferContext, GPUBuffer* gpuBuffer, void* data, Uint32 data_size) {
    TransferContextInfo transferContextInfo = {0};

    for (int i = 0; i < transferContext->transferBuffers.len; i++) {
        TransferBufferInfo currTbInfo = transferContext->transferBuffers.arr[i];

        if (i == transferContext->transferBuffers.len - 1 && currTbInfo.amountFilled + data_size > currTbInfo.bufferSize) {
            TransferBufferInfo tbInfo = {0};
            tbInfo.bufferSize = TRANSFER_BUFFER_SIZE;
            tbInfo.amountFilled = 0;
            tbInfo.transferBuffer = SDL_CreateGPUTransferBuffer(
                transferContext->device,
                &(SDL_GPUTransferBufferCreateInfo){
                    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                    .size = tbInfo.bufferSize,
                    .props = 0,
                });

            if (tbInfo.transferBuffer == NULL) {
                SDL_Log("Failed to allocate memory for transfer buffer: %s", SDL_GetError());
                return -1;
            }

            DYNAMIC_ARRAY_APPEND(transferContext->transferBuffers, tbInfo);

            transferContextInfo.tbIndex = transferContext->transferBuffers.len - 1;
            transferContextInfo.tbOffset = transferContext->transferBuffers.arr[transferContext->transferBuffers.len - 1].amountFilled;
            // Have to use pointers to modify the original value in the array and not the local stack variable tbInfo
            // (I've made this mistake a lot today, so I'm adding this as a reminder)
            transferContext->transferBuffers.arr[i].amountFilled += data_size;
            break;
        } else if (currTbInfo.amountFilled + data_size > currTbInfo.bufferSize) {
            continue;
        } else {
            transferContextInfo.tbIndex = i;
            transferContextInfo.tbOffset = currTbInfo.amountFilled;
            // Have to use pointers to modify the original value in the array and not the local stack variable tbInfo
            // (I've made this mistake a lot today, so I'm adding this as a reminder)
            transferContext->transferBuffers.arr[i].amountFilled += data_size;
            break;
        }
    }

    GPUBUFFER_ACQUIRE_REFERENCE(gpuBuffer);
    transferContextInfo.gpuBuffer = gpuBuffer;

    // This is done to ensure the data persists in memory by upload time, as the provided data pointer
    // be de-allocated by the program before GPUTransferContextSubmit is called, and mapping of the data
    // to the transfer buffer is handled then
    transferContextInfo.data = MemoryArena_allocate(&transferContext->dataStagingArena, data_size);
    SDL_memcpy(transferContextInfo.data, data, data_size);

    transferContextInfo.dataSize = data_size;

    DYNAMIC_ARRAY_APPEND(transferContext->contextInfo, transferContextInfo);

    return 0;
}