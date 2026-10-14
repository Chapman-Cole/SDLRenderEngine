set(SHADER_SOURCES
    perlin.slang
)

# This variable is used to get accurate tracking with the percentages displayed by cmake
set(SHADER_SOURCES_OUTPUT "")

foreach(SHADER IN LISTS SHADER_SOURCES)
    set(SHADER_DIR_PATH "${CMAKE_SOURCE_DIR}/shaders/runtime")
    cmake_path(GET SHADER STEM SHADER_NAME)

    set(VERTEX_SPIRV_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_NAME}.vert.spv")
    set(FRAGMENT_SPIRV_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_NAME}.frag.spv")

    list(APPEND SHADER_SOURCES_OUTPUT ${VERTEX_SPIRV_OUTPUT})
    list(APPEND SHADER_SOURCES_OUTPUT ${FRAGMENT_SPIRV_OUTPUT})
    
    # Generate the spirv vertex shader code
    add_custom_command(
        OUTPUT ${VERTEX_SPIRV_OUTPUT}
        COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target spirv -profile glsl_450 -stage vertex -entry vertexMain -o ${VERTEX_SPIRV_OUTPUT}
        COMMENT "Compiling Shader: ${SHADER} -> ${VERTEX_SPIRV_OUTPUT}"
        DEPENDS ${SHADER_DIR_PATH}/${SHADER} # Re-run the command when the shader source changes
    )

    # Generate the spirv fragment shader code
    add_custom_command(
        OUTPUT ${FRAGMENT_SPIRV_OUTPUT}
        COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target spirv -profile glsl_450 -stage fragment -entry fragmentMain -o ${FRAGMENT_SPIRV_OUTPUT}
        COMMENT "Compiling Shader: ${SHADER} -> ${FRAGMENT_SPIRV_OUTPUT}"
        DEPENDS ${SHADER_DIR_PATH}/${SHADER} # Re-run the command when the shader source changes
    )

    if(WIN32)
        set(VERTEX_DXIL_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_NAME}.vert.dxil")
        set(FRAGMENT_DXIL_OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_NAME}.frag.dxil")

        list(APPEND SHADER_SOURCES_OUTPUT ${VERTEX_DXIL_OUTPUT})
        list(APPEND SHADER_SOURCES_OUTPUT ${FRAGMENT_DXIL_OUTPUT})

        # Generate the dxil vertex shader code
        add_custom_command(
            OUTPUT ${VERTEX_DXIL_OUTPUT}
            COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target dxil -profile sm_6_0 -stage vertex -entry vertexMain -o ${VERTEX_DXIL_OUTPUT}
            COMMENT "Compiling Shader: ${SHADER} -> ${VERTEX_DXIL_OUTPUT}"
            DEPENDS ${SHADER_DIR_PATH}/${SHADER}
        )

        # Generate the dxil fragment shader code
        add_custom_command(
            OUTPUT ${FRAGMENT_DXIL_OUTPUT}
            COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target dxil -profile sm_6_0 -stage fragment -entry fragmentMain -o ${FRAGMENT_DXIL_OUTPUT}
            COMMENT "Compiling Shader: ${SHADER} -> ${FRAGMENT_DXIL_OUTPUT}"
            DEPENDS ${SHADER_DIR_PATH}/${SHADER}
        )
    endif()
endforeach()

add_custom_target(CompiledShaders ALL DEPENDS ${SHADER_SOURCES_OUTPUT})