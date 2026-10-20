add_executable(main main.c)

find_package(SDL3 REQUIRED)

if (ENABLE_ASAN)
    if(WIN32)
        target_compile_options(main PRIVATE /fsanitize=address)
        target_link_options(main PRIVATE /INCREMENTAL:NO)
    elseif(UNIX)
        target_compile_options(main PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
        target_link_options(main PRIVATE -fsanitize=address)
    endif()
endif()

target_include_directories(main PUBLIC ${sdl_ttf_header_path})

target_link_libraries(main PRIVATE RenderEngine)

add_dependencies(main RenderEngine)

# ----------------------------------------------------------------

add_executable(claytest claytest.c)

if (ENABLE_ASAN)
    if(WIN32)
        target_compile_options(claytest PRIVATE /fsanitize=address)
        target_link_options(claytest PRIVATE /INCREMENTAL:NO)
    elseif(UNIX)
        target_compile_options(claytest PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
        target_link_options(claytest PRIVATE -fsanitize=address)
    endif()
endif()

target_include_directories(claytest PUBLIC ${sdl_ttf_header_path} ${CMAKE_SOURCE_DIR}/external/clay ${CMAKE_SOURCE_DIR}/ClayCompositor/include)

target_link_libraries(claytest PRIVATE RenderEngine ClayCompositor)

add_dependencies(claytest RenderEngine ClayCompositor)