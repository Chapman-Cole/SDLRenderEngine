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
    src/MemoryHeap.c
    src/HashMap.c
 )

if (ENABLE_ASAN)
    if(WIN32)
        target_compile_options(RenderEngine PRIVATE /fsanitize=address)
        target_link_options(RenderEngine PRIVATE /INCREMENTAL:NO)
    elseif(UNIX)
        target_compile_options(RenderEngine PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
        target_link_options(RenderEngine PRIVATE -fsanitize=address)
    endif()
endif()

find_package(SDL3 REQUIRED)

# Vulkan is only required to get the macro for making the api version
find_package(Vulkan REQUIRED)

# Find SDL_ttf on system path
find_package(SDL3_ttf REQUIRED)

target_include_directories(RenderEngine PUBLIC SDL3_ttf::SDL3_ttf "include/" ${CMAKE_SOURCE_DIR})

target_link_libraries(RenderEngine PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf PRIVATE Vulkan::Vulkan)

add_dependencies(RenderEngine BuiltinShaders CompiledShaders)

