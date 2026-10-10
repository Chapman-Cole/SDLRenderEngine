add_library(ClayCompositor STATIC
    src/ClayCompositor.c
)

target_include_directories(ClayCompositor PUBLIC ${sdl_ttf_header_path} "include/" ${CMAKE_SOURCE_DIR}/RenderEngine/include ${CMAKE_SOURCE_DIR}/ClayCompositor/include ${CMAKE_SOURCE_DIR}/external/clay)

target_link_libraries(ClayCompositor PUBLIC SDL3 ${sdl_ttf_library_path} RenderEngine)

add_dependencies(RenderEngine RenderEngine)