add_executable(main main.c)

if (ENABLE_ASAN)
    target_compile_options(main PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(main PRIVATE -fsanitize=address)
endif()

target_include_directories(main PUBLIC ${sdl_ttf_header_path})

target_link_libraries(main PUBLIC SDL3 ${sdl_ttf_library_path} PRIVATE RenderEngine)

add_dependencies(main RenderEngine)

# ---------------------------------------------------

add_executable(claytest claytest.c)

if (ENABLE_ASAN)
    target_compile_options(claytest PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(claytest PRIVATE -fsanitize=address)
endif()

target_include_directories(claytest PUBLIC ${sdl_ttf_header_path} ${CMAKE_SOURCE_DIR}/external/clay ${CMAKE_SOURCE_DIR}/ClayCompositor/include)

target_link_libraries(claytest PUBLIC ${sdl_ttf_library_path} PRIVATE RenderEngine ClayCompositor)