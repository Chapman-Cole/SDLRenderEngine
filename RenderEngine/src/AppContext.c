#include "AppContext.h"
#include "shaders/builtin/headers/circle_fragment_spirv.h"
#include "shaders/builtin/headers/circle_vertex_spirv.h"
#include "shaders/builtin/headers/rectangle_fragment_spirv.h"
#include "shaders/builtin/headers/rectangle_vertex_spirv.h"
#include "shaders/builtin/headers/text_fragment_spirv.h"
#include "shaders/builtin/headers/text_vertex_spirv.h"
#include "shaders/builtin/headers/textcursor_fragment_spirv.h"
#include "shaders/builtin/headers/textcursor_vertex_spirv.h"
#include "vulkan/vulkan.h"

#ifdef _WIN32
#include "shaders/builtin/headers/circle_fragment_dxil.h"
#include "shaders/builtin/headers/circle_vertex_dxil.h"
#include "shaders/builtin/headers/rectangle_fragment_dxil.h"
#include "shaders/builtin/headers/rectangle_vertex_dxil.h"
#include "shaders/builtin/headers/text_fragment_dxil.h"
#include "shaders/builtin/headers/text_vertex_dxil.h"
#include "shaders/builtin/headers/textcursor_fragment_dxil.h"
#include "shaders/builtin/headers/textcursor_vertex_dxil.h"
#endif

static SDL_GPUColorTargetBlendState DefaultBlendState = {
    .enable_blend = true,
    .color_blend_op = SDL_GPU_BLENDOP_ADD,
    .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
    .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
    .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
    .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
    .enable_color_write_mask = 0,
};

static inline int AppContextLoadRectanglePipeline(AppContext* app_context) {
    size_t vertex_shader_source_len, fragment_shader_source_len;
    Uint8* vertex_shader_source;
    Uint8* fragment_shader_source;

    switch (app_context->shaderFormat) {
    case SDL_GPU_SHADERFORMAT_SPIRV:
        vertex_shader_source_len = rectangle_vertex_spirv_source_len;
        vertex_shader_source = (Uint8*)rectangle_vertex_spirv_source;

        fragment_shader_source_len = rectangle_fragment_spirv_source_len;
        fragment_shader_source = (Uint8*)rectangle_fragment_spirv_source;
        break;

    case SDL_GPU_SHADERFORMAT_DXIL:
        vertex_shader_source_len = rectangle_vertex_dxil_source_len;
        vertex_shader_source = (Uint8*)rectangle_vertex_dxil_source;

        fragment_shader_source_len = rectangle_fragment_dxil_source_len;
        fragment_shader_source = (Uint8*)rectangle_fragment_dxil_source;
        break;

    default:
        break;
    }

    SDL_GPUShader* rectangleVertexShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = vertex_shader_source,
            .code_size = vertex_shader_source_len,
            .format = app_context->shaderFormat,
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
            .code = fragment_shader_source,
            .code_size = fragment_shader_source_len,
            .format = app_context->shaderFormat,
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
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(app_context->device, app_context->window),
                    .blend_state = DefaultBlendState,
                }},
            },
            .vertex_input_state = (SDL_GPUVertexInputState){
                .num_vertex_buffers = 0,
                .vertex_buffer_descriptions = NULL,
                .num_vertex_attributes = 0,
                .vertex_attributes = NULL,
            },
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
    size_t vertex_shader_source_len, fragment_shader_source_len;
    Uint8* vertex_shader_source;
    Uint8* fragment_shader_source;

    switch (app_context->shaderFormat) {
    case SDL_GPU_SHADERFORMAT_SPIRV:
        vertex_shader_source_len = circle_vertex_spirv_source_len;
        vertex_shader_source = (Uint8*)circle_vertex_spirv_source;

        fragment_shader_source_len = circle_fragment_spirv_source_len;
        fragment_shader_source = (Uint8*)circle_fragment_spirv_source;
        break;

    case SDL_GPU_SHADERFORMAT_DXIL:
        vertex_shader_source_len = circle_vertex_dxil_source_len;
        vertex_shader_source = (Uint8*)circle_vertex_dxil_source;

        fragment_shader_source_len = circle_fragment_dxil_source_len;
        fragment_shader_source = (Uint8*)circle_fragment_dxil_source;
        break;

    default:
        break;
    }

    SDL_GPUShader* circleVertexShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = vertex_shader_source,
            .code_size = vertex_shader_source_len,
            .format = app_context->shaderFormat,
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
            .code = fragment_shader_source,
            .code_size = fragment_shader_source_len,
            .format = app_context->shaderFormat,
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
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(app_context->device, app_context->window),
                    .blend_state = DefaultBlendState,
                }},
            },
            .vertex_input_state = (SDL_GPUVertexInputState){
                .num_vertex_buffers = 0,
                .vertex_buffer_descriptions = NULL,
                .num_vertex_attributes = 0,
                .vertex_attributes = NULL,
            },
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

static inline int AppContextLoadTextPipeline(AppContext* app_context) {
    size_t vertex_shader_source_len, fragment_shader_source_len;
    Uint8* vertex_shader_source;
    Uint8* fragment_shader_source;

    switch (app_context->shaderFormat) {
    case SDL_GPU_SHADERFORMAT_SPIRV:
        vertex_shader_source_len = text_vertex_spirv_source_len;
        vertex_shader_source = (Uint8*)text_vertex_spirv_source;

        fragment_shader_source_len = text_fragment_spirv_source_len;
        fragment_shader_source = (Uint8*)text_fragment_spirv_source;
        break;

    case SDL_GPU_SHADERFORMAT_DXIL:
        vertex_shader_source_len = text_vertex_dxil_source_len;
        vertex_shader_source = (Uint8*)text_vertex_dxil_source;

        fragment_shader_source_len = text_fragment_dxil_source_len;
        fragment_shader_source = (Uint8*)text_fragment_dxil_source;
        break;

    default:
        break;
    }

    SDL_GPUShader* textVertexShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = vertex_shader_source,
            .code_size = vertex_shader_source_len,
            .format = app_context->shaderFormat,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1, // Update this as needed
            .props = 0,
            .entrypoint = "main",
        });

    if (textVertexShader == NULL) {
        SDL_Log("Failed to create textVertexShader: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUShader* textFragmentShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = fragment_shader_source,
            .code_size = fragment_shader_source_len,
            .format = app_context->shaderFormat,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 1,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1, // Update this as needed
            .props = 0,
            .entrypoint = "main",
        });

    if (textFragmentShader == NULL) {
        SDL_ReleaseGPUShader(app_context->device, textVertexShader);
        SDL_Log("Failed to create textFragmentShader: %s", SDL_GetError());
        return -1;
    }

    app_context->builtinPipelines[BUILTIN_PIPELINE_TEXT] = SDL_CreateGPUGraphicsPipeline(
        app_context->device,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(app_context->device, app_context->window),
                    .blend_state = DefaultBlendState,
                }},
            },
            .vertex_input_state = (SDL_GPUVertexInputState){
                .num_vertex_buffers = 1,
                .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){
                    {
                        .slot = 0,
                        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                        .instance_step_rate = 0,
                        .pitch = 2 * sizeof(SDL_FPoint),
                    },
                },
                .num_vertex_attributes = 2,
                .vertex_attributes = (SDL_GPUVertexAttribute[]){
                    {
                        .buffer_slot = 0,
                        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                        .location = 0,
                        .offset = 0,
                    },
                    {
                        .buffer_slot = 0,
                        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                        .location = 1,
                        .offset = 2 * sizeof(float),
                    },
                },
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = textVertexShader,
            .fragment_shader = textFragmentShader,
        });

    SDL_ReleaseGPUShader(app_context->device, textVertexShader);
    SDL_ReleaseGPUShader(app_context->device, textFragmentShader);

    if (app_context->builtinPipelines[BUILTIN_PIPELINE_TEXT] == NULL) {
        SDL_Log("Failed to create text graphics pipeline: %s", SDL_GetError());
        return false;
    }

    return true;
}

static inline int AppContextLoadTextCursorPipeline(AppContext* app_context) {
    size_t vertex_shader_source_len, fragment_shader_source_len;
    Uint8* vertex_shader_source;
    Uint8* fragment_shader_source;

    switch (app_context->shaderFormat) {
    case SDL_GPU_SHADERFORMAT_SPIRV:
        vertex_shader_source_len = textcursor_vertex_spirv_source_len;
        vertex_shader_source = (Uint8*)textcursor_vertex_spirv_source;

        fragment_shader_source_len = textcursor_fragment_spirv_source_len;
        fragment_shader_source = (Uint8*)textcursor_fragment_spirv_source;
        break;

    case SDL_GPU_SHADERFORMAT_DXIL:
        vertex_shader_source_len = textcursor_vertex_dxil_source_len;
        vertex_shader_source = (Uint8*)textcursor_vertex_dxil_source;

        fragment_shader_source_len = textcursor_fragment_dxil_source_len;
        fragment_shader_source = (Uint8*)textcursor_fragment_dxil_source;
        break;

    default:
        break;
    }

    SDL_GPUShader* textCursorVertexShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = vertex_shader_source,
            .code_size = vertex_shader_source_len,
            .format = app_context->shaderFormat,
            .stage = SDL_GPU_SHADERSTAGE_VERTEX,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1, // Update this as needed
            .props = 0,
            .entrypoint = "main"});

    if (textCursorVertexShader == NULL) {
        SDL_Log("Failed to create textCursorVertexShader: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUShader* textCursorFragmentShader = SDL_CreateGPUShader(
        app_context->device,
        &(SDL_GPUShaderCreateInfo){
            .code = fragment_shader_source,
            .code_size = fragment_shader_source_len,
            .format = app_context->shaderFormat,
            .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
            .num_samplers = 0,
            .num_storage_buffers = 0,
            .num_storage_textures = 0,
            .num_uniform_buffers = 1, // Update this as needed
            .props = 0,
            .entrypoint = "main",
        });

    if (textCursorFragmentShader == NULL) {
        SDL_ReleaseGPUShader(app_context->device, textCursorVertexShader);
        SDL_Log("Failed to create textCursorFragmentShader: %s", SDL_GetError());
        return -1;
    }

    app_context->builtinPipelines[BUILTIN_PIPELINE_TEXT_CURSOR] = SDL_CreateGPUGraphicsPipeline(
        app_context->device,
        &(SDL_GPUGraphicsPipelineCreateInfo){
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(app_context->device, app_context->window),
                    .blend_state = DefaultBlendState,
                }},
            },
            .vertex_input_state = (SDL_GPUVertexInputState){
                .num_vertex_buffers = 1,
                .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){
                    {
                        .slot = 0,
                        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                        .instance_step_rate = 0,
                        .pitch = 2 * sizeof(SDL_FPoint),
                    },
                },
                .num_vertex_attributes = 2,
                .vertex_attributes = (SDL_GPUVertexAttribute[]){
                    {
                        .buffer_slot = 0,
                        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                        .location = 0,
                        .offset = 0,
                    },
                    {
                        .buffer_slot = 0,
                        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
                        .location = 1,
                        .offset = 2 * sizeof(float),
                    },
                },
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = textCursorVertexShader,
            .fragment_shader = textCursorFragmentShader,
        });

    SDL_ReleaseGPUShader(app_context->device, textCursorVertexShader);
    SDL_ReleaseGPUShader(app_context->device, textCursorFragmentShader);

    if (app_context->builtinPipelines[BUILTIN_PIPELINE_TEXT_CURSOR] == NULL) {
        SDL_Log("Failed to create text cursor graphics pipeline: %s", SDL_GetError());
        return false;
    }

    return true;
}

int AppContextInit(AppContext* app_context, SDL_WindowFlags windowFlags, bool gpuDebugEnabled, size_t transferContextArenaSize) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize video subsystem: %s", SDL_GetError());
        return -1;
    }

    if (app_context->deviceName == NULL) {
        SDL_Log("ERROR: No device driver name was specified.\n");
        return -1;
    }

    SDL_PropertiesID gpuProps = SDL_CreateProperties();
    SDL_SetStringProperty(gpuProps, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, (const char*)app_context->deviceName);
    SDL_SetBooleanProperty(gpuProps, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, gpuDebugEnabled);

    SDL_GPUVulkanOptions vkOptions = {0};
    if (SDL_strcmp(app_context->deviceName, APP_CONTEXT_DEVICE_DRIVER_VULKAN) == 0) {
        vkOptions.vulkan_api_version = VK_MAKE_API_VERSION(0, 1, 4, 0);

        // In order for SV_VertexID in slang to work, shader draw paremeters extension must be enabled
        VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParameters = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
            .pNext = NULL,
            .shaderDrawParameters = VK_TRUE,
        };
        vkOptions.feature_list = &shaderDrawParameters;
        SDL_SetPointerProperty(gpuProps, SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER, &vkOptions);

        app_context->shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
        SDL_SetBooleanProperty(gpuProps, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    } else if (SDL_strcmp(app_context->deviceName, APP_CONTEXT_DEVICE_DRIVER_DIRECT3D12) == 0) {
        app_context->shaderFormat = SDL_GPU_SHADERFORMAT_DXIL;
        SDL_SetBooleanProperty(gpuProps, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    } else {
        SDL_Log("ERROR: Unsupported device driver: %s", app_context->deviceName);
        return -1;
    }

    app_context->device = SDL_CreateGPUDeviceWithProperties(gpuProps);
    SDL_DestroyProperties(gpuProps);

    if (app_context->device == NULL) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        return -1;
    }

    app_context->window = SDL_CreateWindow(app_context->windowName, 640, 480, windowFlags);
    if (app_context->window == NULL) {
        SDL_Log("Failed to create window for app context: %s\n", SDL_GetError());
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(app_context->device, app_context->window)) {
        SDL_Log("Failed to claim window for GPU: %s", SDL_GetError());
        return -1;
    }

    if (!TTF_Init()) {
        SDL_Log("Failed to initialize SDL3_ttf: %s", SDL_GetError());
        return -1;
    }

    app_context->textEngine = TTF_CreateGPUTextEngine(app_context->device);
    if (app_context->textEngine == NULL) {
        SDL_Log("Failed to create gpu text engine: %s", SDL_GetError());
        return -1;
    }

    app_context->textSampler = SDL_CreateGPUSampler(
        app_context->device,
        &(SDL_GPUSamplerCreateInfo){
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        });

    app_context->displayScale = SDL_GetWindowDisplayScale(app_context->window);

    if (!AppContextLoadRectanglePipeline(app_context)) {
        return -1;
    }

    if (!AppContextLoadCirclePipeline(app_context)) {
        return -1;
    }

    if (!AppContextLoadTextPipeline(app_context)) {
        return -1;
    }

    if (!AppContextLoadTextCursorPipeline(app_context)) {
        return -1;
    }

    if (GPUTransferContextCreate(&app_context->transferContext, APP_CONTEXT_DEVICE(*app_context), transferContextArenaSize) < 0) {
        return -1;
    }

    return 0;
}

int AppContextTerminate(AppContext* app_context) {
    GPUTransferContextDestroy(&app_context->transferContext);

    SDL_ReleaseGPUSampler(app_context->device, app_context->textSampler);

    SDL_ReleaseGPUGraphicsPipeline(app_context->device, app_context->builtinPipelines[BUILTIN_PIPELINE_RECTANGLE]);
    SDL_ReleaseGPUGraphicsPipeline(app_context->device, app_context->builtinPipelines[BUILTIN_PIPELINE_CIRCLE]);
    SDL_ReleaseGPUGraphicsPipeline(app_context->device, app_context->builtinPipelines[BUILTIN_PIPELINE_TEXT]);
    SDL_ReleaseGPUGraphicsPipeline(app_context->device, app_context->builtinPipelines[BUILTIN_PIPELINE_TEXT_CURSOR]);

    TTF_DestroyGPUTextEngine(app_context->textEngine);
    TTF_Quit();

    SDL_ReleaseWindowFromGPUDevice(app_context->device, app_context->window);
    SDL_DestroyWindow(app_context->window);
    SDL_DestroyGPUDevice(app_context->device);

    return 0;
}