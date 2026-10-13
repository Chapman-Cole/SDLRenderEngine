add_library(RenderEngine STATIC
    src/AppContext.c 
    src/RenderContext.c 
    src/Shaders.c 
    src/GPUTransferContext.c 
    src/DrawFunctions.c 
    src/GPUText.c
    src/FontRegistry.c
    src/InputContext.c
    src/MemoryArena.c
)

if (ENABLE_ASAN)
    if(WIN32)
        target_compile_options(RenderEngine PRIVATE /fsanitize=address)
    elseif(UNIX)
        target_compile_options(RenderEngine PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
        target_link_options(RenderEngine PRIVATE -fsanitize=address)
    endif()
endif()

find_package(SDL3 REQUIRED)

# Vulkan is only required to get the macro for making the api version
find_package(Vulkan REQUIRED)

# Find SDL_ttf on system path
find_path(sdl_ttf_header_path NAMES SDL3_ttf)
if(NOT DEFINED sdl_ttf_header_path)
    message(FATAL_ERROR "Could not find SDL3_ttf headers on system path.")
endif()

find_library(sdl_ttf_library_path NAMES SDL3_ttf)
if (NOT DEFINED sdl_ttf_library_path)
    message(FATAL_ERROR "Could not find SDL3_ttf library path.")
endif()

target_include_directories(RenderEngine PUBLIC ${sdl_ttf_header_path} "include/" ${CMAKE_SOURCE_DIR})

target_link_libraries(RenderEngine PUBLIC SDL3::SDL3 ${sdl_ttf_library_path} PRIVATE Vulkan::Vulkan)

add_dependencies(RenderEngine BuiltinShaders CompiledShaders)

