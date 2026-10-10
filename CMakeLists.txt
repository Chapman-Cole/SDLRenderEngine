cmake_minimum_required(VERSION 3.23)
project(SDLRenderEngine LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(shaders)

add_subdirectory(RenderEngine)

add_subdirectory(ClayCompositor)

add_subdirectory(examples)
