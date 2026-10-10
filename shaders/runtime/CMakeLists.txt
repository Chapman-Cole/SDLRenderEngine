set(SHADER_SOURCES
    clear.vert
    clear.frag
)       

# This variable is used to get accurate tracking with the percentages displayed by cmake
set(SHADER_SOURCES_OUTPUT "")

foreach(SHADER IN LISTS SHADER_SOURCES)
    set(OUTPUT_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER}.spv")

    list(APPEND SHADER_SOURCES_OUTPUT ${OUTPUT_FILE})

    set(SHADER_DIR_PATH "${CMAKE_SOURCE_DIR}/shaders/runtime")
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND glslc ${SHADER_DIR_PATH}/${SHADER} -o ${OUTPUT_FILE}
        COMMENT "Compiling Shader: ${SHADER} -> ${OUTPUT_FILE}"
        DEPENDS ${SHADER_DIR_PATH}/${SHADER} # Re-run the command when the shader source changes
    )
endforeach()

add_custom_target(CompiledShaders ALL DEPENDS ${SHADER_SOURCES_OUTPUT})