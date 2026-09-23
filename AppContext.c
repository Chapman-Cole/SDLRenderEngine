#include "AppContext.h"
#include "shaders/builtin/headers/rectangle.vert.h"
#include "shaders/builtin/headers/rectangle.frag.h"
#include "shaders/builtin/headers/circle.vert.h"
#include "shaders/builtin/headers/circle.frag.h"

static inline int AppContextLoadRectanglePipeline(AppContext* app_context) {
    SDL_GPUShader* rectangleVertexShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = rectangle_vert_source,
            .code_size = rectangle_vert_source_len,
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1,
            .props = 0,
            .entrypoint = "main"});

    if (rectangleVertexShader == NULL) {
        SDL_Log("Failed to create builtin rectangle vertex shader: %s\n", SDL_GetError());
        return false;
    }

    SDL_GPUShader* rectangleFragmentShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = rectangle_frag_source,
            .code_size = rectangle_frag_source_len  ,
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1,
            .props = 0,
            .entrypoint = "main"});

    if (rectangleFragmentShader == NULL) {
        SDL_Log("Failed to create builtin rectangle fragment shader: %s\n", SDL_GetError());
        SDL_ReleaseGPUShader(app_context->device, rectangleVertexShader);
        return false;
    }

    app_context->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE] = SDL_CreateGPUGraphicsPipeline(
        app_context->device,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(app_context->device, app_context->window)}},
            },
            .vertex_input_state = (SDL_GPUVertexInputState){.num_vertex_buffers = 0, .vertex_buffer_descriptions = NULL, .num_vertex_attributes = 0, .vertex_attributes = NULL},
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = rectangleVertexShader,
            .fragment_shader = rectangleFragmentShader,
        });

    SDL_ReleaseGPUShader(app_context->device, rectangleVertexShader);
    SDL_ReleaseGPUShader(app_context->device, rectangleFragmentShader);

    if (app_context->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE] == NULL) {
        SDL_Log("Failed to create builtin rectangle graphics pipeline: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

static inline int AppContextLoadCirclePipeline(AppContext* app_context) {
    SDL_GPUShader* circleVertexShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = circle_vert_source,
            .code_size = circle_vert_source_len,
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1,
            .props = 0,
            .entrypoint = "main"});

    if (circleVertexShader == NULL) {
        SDL_Log("Failed to create builtin rectangle vertex shader: %s\n", SDL_GetError());
        return false;
    }

    SDL_GPUShader* circleFragmentShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = circle_frag_source,
            .code_size = circle_frag_source_len  ,
            .format = SDL_GPU_SHADERFORMAT_SPIRV,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1,
            .props = 0,
            .entrypoint = "main"});

    if (circleFragmentShader == NULL) {
        SDL_Log("Failed to create builtin rectangle fragment shader: %s\n", SDL_GetError());
        SDL_ReleaseGPUShader(app_context->device, circleVertexShader);
        return false;
    }

    app_context->builtinPipelines[BUILTIN_PIPELINE_CIRCLE] = SDL_CreateGPUGraphicsPipeline(
        app_context->device,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(app_context->device, app_context->window)}},
            },
            .vertex_input_state = (SDL_GPUVertexInputState){.num_vertex_buffers = 0, .vertex_buffer_descriptions = NULL, .num_vertex_attributes = 0, .vertex_attributes = NULL},
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = circleVertexShader,
            .fragment_shader = circleFragmentShader,
        });

    SDL_ReleaseGPUShader(app_context->device, circleVertexShader);
    SDL_ReleaseGPUShader(app_context->device, circleFragmentShader);

    if (app_context->builtinPipelines[BUILTIN_PIPELINE_CIRCLE] == NULL) {
        SDL_Log("Failed to create builtin circle graphics pipeline: %s\n", SDL_GetError());
        return false;
    }

    return true;  
}

int AppContextInit(AppContext* app_context, SDL_WindowFlags windowFlags) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize video subsystem.\n");
        return -1;
    }

    app_context->device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, app_context->deviceName);
    if (app_context->device == NULL) {
        SDL_Log("Failed to create GPU device.\n");
        return -1;
    }

    app_context->window = SDL_CreateWindow(app_context->windowName, 640, 480, windowFlags);
    if (app_context->window == NULL) {
        SDL_Log("Failed to create window for app context: %s\n", SDL_GetError());
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(app_context->device, app_context->window)) {
        SDL_Log("Failed to claim window for GPU.\n");
        return -1;
    }

    if (!AppContextLoadRectanglePipeline(app_context)) {
        return -1;
    }

    if (!AppContextLoadCirclePipeline(app_context)) {
        return -1;
    }

    return 0;
}

int AppContextTerminate(AppContext* app_context) {
    SDL_ReleaseGPUGraphicsPipeline(app_context->device, app_context->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);
    SDL_ReleaseWindowFromGPUDevice(app_context->device, app_context->window);
    SDL_DestroyWindow(app_context->window);
    SDL_DestroyGPUDevice(app_context->device);
    return 0;
}