#include "Shaders.h"
#include "AppContext.h"

// Code references and uses part of the official SDL gpu api examples github, specifically in common.c
// https://github.com/TheSpydog/SDL_gpu_examples/blob/main/Examples/Common.c
SDL_GPUShader* ShaderLoad(AppContext* appContext, const char* path, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount) {
    SDL_GPUShaderStage stage;
    if (SDL_strstr(path, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(path, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid shader stage.\n");
        return NULL;
    }

    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(appContext->device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    // Right now I only plan on supporting SPIRV for simplicity
    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    }

    size_t shaderFileSize;
    void* shaderCode = SDL_LoadFile(path, &shaderFileSize);
    if (shaderCode == NULL) {
        SDL_Log("Failed to load shader from disk: %s", path);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code = shaderCode,
        .code_size = shaderFileSize,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_uniform_buffers = uniformBufferCount,
        .num_storage_buffers = storageBufferCount,
        .num_storage_textures = storageTextureCount};

    SDL_GPUShader* shader = SDL_CreateGPUShader(appContext->device, &shaderInfo);
    if (shader == NULL) {
        SDL_Log("Failed to create shader.\n");
        SDL_free(shaderCode);
        return NULL;
    }

    SDL_free(shaderCode);
    return shader;
}

SDL_GPUShader* ShaderLoadText(AppContext* appContext, const char* shaderCode, Uint32 samplerCount, Uint32 uniformBufferCount, Uint32 storageBufferCount, Uint32 storageTextureCount) {
    SDL_GPUShaderStage stage;
    if (SDL_strstr(shaderCode, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(shaderCode, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid shader stage.\n");
        return NULL;
    }

    SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(appContext->device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    // Right now I only plan on supporting SPIRV for simplicity
    if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    }

    SDL_GPUShaderCreateInfo shaderInfo = {
        .code = (Uint8*)shaderCode,
        .code_size = SDL_strlen(shaderCode),
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = samplerCount,
        .num_uniform_buffers = uniformBufferCount,
        .num_storage_buffers = storageBufferCount,
        .num_storage_textures = storageTextureCount};

    SDL_GPUShader* shader = SDL_CreateGPUShader(appContext->device, &shaderInfo);
    if (shader == NULL) {
        SDL_Log("Failed to create shader.\n");
        return NULL;
    }

    return shader;
}

SDL_GPUGraphicsPipeline* GraphicsPipelineCreate(AppContext* appContext, SDL_GPUShader* vertexShader, SDL_GPUShader* fragmentShader) {
    // For now, the graphics pipeline by default will only expect a position stream of three floats (no other vertex attributes at the moment)
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(appContext->device, appContext->window)}},
        },
        .vertex_input_state = (SDL_GPUVertexInputState){.num_vertex_buffers = 1, .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{.slot = 0, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0, .pitch = 3 * sizeof(float)}}, .num_vertex_attributes = 1, .vertex_attributes = (SDL_GPUVertexAttribute[]){{.buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .location = 0, .offset = 0}}},
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertexShader,
        .fragment_shader = fragmentShader,
    };

    pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    SDL_GPUGraphicsPipeline* graphicsPipeline = SDL_CreateGPUGraphicsPipeline(appContext->device, &pipelineCreateInfo);
    if (graphicsPipeline == NULL) {
        SDL_Log("Failed to create graphics pipeline.\n");
        return NULL;
    }

    SDL_ReleaseGPUShader(appContext->device, vertexShader);
    SDL_ReleaseGPUShader(appContext->device, fragmentShader);

    return graphicsPipeline;
}
