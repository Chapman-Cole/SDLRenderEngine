#ifndef SHADERS_H
#define SHADERS_H

#include "SDL3/SDL.h"
#include "AppContext.h"

SDL_GPUShader* ShaderLoad(AppContext* appContext, const char* path, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount);

SDL_GPUShader* ShaderLoadText(AppContext* appContext, const char* shaderCode, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount);

// For now, it will assume the target is the swapchain texture, but this needs to be updated in the future to accomdate
// arbitrary gpu textures. Also, this function releases the provided shaders automatically
SDL_GPUGraphicsPipeline* GraphicsPipelineCreate(AppContext* appContext, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader);

#endif