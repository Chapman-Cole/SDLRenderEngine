cmake_minimum_required(VERSION 3.23)
project(SDLRenderEngine LANGUAGES C)

# Handle shader source compilation

set(SHADER_SOURCES
    clear.vert
    clear.frag
)

# This variable is used to get accurate tracking with the percentages displayed by cmake
set(SHADER_SOURCES_OUTPUT "")

foreach(SHADER IN LISTS SHADER_SOURCES)
    set(OUTPUT_FILE "${CMAKE_BINARY_DIR}/${SHADER}.spv")

    list(APPEND SHADER_SOURCES_OUTPUT ${OUTPUT_FILE})
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND glslc ${CMAKE_SOURCE_DIR}/shaders/${SHADER} -o ${OUTPUT_FILE}
        COMMENT "Compiling Shader: ${SHADER} -> ${OUTPUT_FILE}"
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/${SHADER} # Re-run the command when the shader source changes
    )
endforeach()

add_custom_target(CompiledShaders ALL DEPENDS ${SHADER_SOURCES_OUTPUT})

# Handle builtin shader source compilation below

set(BUILTIN_SHADER_SOURCES
    rectangle.vert
    rectangle.frag
    circle.vert
    circle.frag
)

set(BUILTIN_SHADER_FILES "")

foreach(SHADER IN LISTS BUILTIN_SHADER_SOURCES)
    cmake_path(GET SHADER STEM SHADER_NAME)
    cmake_path(GET SHADER EXTENSION SHADER_EXTENSION_NAME)

    set(SHADER_DIR_PATH "${CMAKE_SOURCE_DIR}/shaders/builtin")

    set(SHADER_VAR_NAME "${SHADER}_source")
    string(REPLACE "." "_" SHADER_VAR_NAME "${SHADER_VAR_NAME}")

    set(OUTPUT_FILE "${SHADER_DIR_PATH}/headers/${SHADER_NAME}${SHADER_EXTENSION_NAME}.h")

    list(APPEND BUILTIN_SHADER_FILES ${OUTPUT_FILE})

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND glslc ${SHADER_DIR_PATH}/${SHADER} -o ${SHADER_DIR_PATH}/${SHADER}.spv
        COMMAND xxd -i -n "${SHADER_VAR_NAME}" ${SHADER_DIR_PATH}/${SHADER}.spv > ${OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${SHADER_DIR_PATH}/${SHADER}.spv # clean up generated SPIR-V files
        COMMENT "Generating SPIR-V Header: ${SHADER} -> ${SHADER_NAME}${SHADER_EXTENSION_NAME}.h"
        DEPENDS "${SHADER_DIR_PATH}/${SHADER}"
    )
endforeach()

add_custom_target(BuiltinShaders ALL DEPENDS ${BUILTIN_SHADER_FILES})

# Handle source files

add_executable(main main.c AppContext.c RenderContext.c Shaders.c GPUBuffers.c DrawFunctions.c)

find_package(SDL3 REQUIRED) 

target_link_libraries(main PRIVATE SDL3)

add_dependencies(main BuiltinShaders CompiledShaders)
