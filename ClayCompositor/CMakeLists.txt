add_library(ClayCompositor STATIC
    src/ClayCompositor.c
)

if (ENABLE_ASAN)
    if(WIN32)
        target_compile_options(ClayCompositor PRIVATE /fsanitize=address)
    elseif(UNIX)
        target_compile_options(ClayCompositor PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
        target_link_options(ClayCompositor PRIVATE -fsanitize=address)
    endif()
endif()

target_include_directories(ClayCompositor PUBLIC ${sdl_ttf_header_path} "include/" ${CMAKE_SOURCE_DIR}/RenderEngine/include ${CMAKE_SOURCE_DIR}/ClayCompositor/include ${CMAKE_SOURCE_DIR}/external/clay)

target_link_libraries(ClayCompositor PUBLIC RenderEngine)

add_dependencies(RenderEngine RenderEngine)