set(BUILTIN_SHADER_SOURCES
    rectangle.vert
    rectangle.frag
    circle.vert
    circle.frag
    text.vert
    text.frag
    textcursor.vert
    textcursor.frag
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