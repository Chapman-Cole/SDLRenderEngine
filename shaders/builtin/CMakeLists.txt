set(BUILTIN_SHADER_SOURCES
    rectangle.slang
    circle.slang
    text.slang
    textcursor.slang
)

set(BUILTIN_SHADER_FILES "")

foreach(SHADER IN LISTS BUILTIN_SHADER_SOURCES)
    cmake_path(GET SHADER STEM SHADER_NAME)

    set(SHADER_DIR_PATH "${CMAKE_SOURCE_DIR}/shaders/builtin")

    set(VERTEX_SPIRV_OUTPUT "${SHADER_DIR_PATH}/headers/${SHADER_NAME}_vertex_spirv.h")

    set(FRAGMENT_SPIRV_OUTPUT "${SHADER_DIR_PATH}/headers/${SHADER_NAME}_fragment_spirv.h")

    list(APPEND BUILTIN_SHADER_FILES ${VERTEX_SPIRV_OUTPUT})
    list (APPEND BUILTIN_SHADER_FILES ${FRAGMENT_SPIRV_OUTPUT})

    # Generate the spirv vertex shader code
    add_custom_command(
        OUTPUT ${VERTEX_SPIRV_OUTPUT}
        COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target spirv -profile glsl_450 -stage vertex -entry vertexMain -o ${SHADER_DIR_PATH}/${SHADER_NAME}.vert.spv
        COMMAND xxd -i -n "${SHADER_NAME}_vertex_spirv_source" ${SHADER_DIR_PATH}/${SHADER_NAME}.vert.spv > ${VERTEX_SPIRV_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${SHADER_DIR_PATH}/${SHADER_NAME}.vert.spv # clean up generated SPIR-V files
        COMMENT "Generating SPIR-V Header: ${SHADER} -> ${SHADER_NAME}_vertex_spirv.h"
        DEPENDS "${SHADER_DIR_PATH}/${SHADER}"
    )

    # Generate the spirv fragment shader code
    add_custom_command(
        OUTPUT ${FRAGMENT_SPIRV_OUTPUT}
        COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target spirv -profile glsl_450 -stage fragment -entry fragmentMain -o ${SHADER_DIR_PATH}/${SHADER_NAME}.frag.spv
        COMMAND xxd -i -n "${SHADER_NAME}_fragment_spirv_source" ${SHADER_DIR_PATH}/${SHADER_NAME}.frag.spv > ${FRAGMENT_SPIRV_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${SHADER_DIR_PATH}/${SHADER_NAME}.frag.spv # clean up generated SPIR-V files
        COMMENT "Generating SPIR-V Header: ${SHADER} -> ${SHADER_NAME}_fragment_spirv.h"
        DEPENDS "${SHADER_DIR_PATH}/${SHADER}"
    )

    set(VERTEX_DXIL_OUTPUT "${SHADER_DIR_PATH}/headers/${SHADER_NAME}_vertex_dxil.h")

    set (FRAGMENT_DXIL_OUTPUT "${SHADER_DIR_PATH}/headers/${SHADER_NAME}_fragment_dxil.h")

    list(APPEND BUILTIN_SHADER_FILES ${VERTEX_DXIL_OUTPUT})
    list (APPEND BUILTIN_SHADER_FILES ${FRAGMENT_DXIL_OUTPUT})

    # Generate the dxil vertex shader code
    add_custom_command(
        OUTPUT ${VERTEX_DXIL_OUTPUT}
        COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target dxil -profile sm_6_0 -stage vertex -entry vertexMain -o ${SHADER_DIR_PATH}/${SHADER_NAME}.vert.dxil
        COMMAND xxd -i -n "${SHADER_NAME}_vertex_dxil_source" ${SHADER_DIR_PATH}/${SHADER_NAME}.vert.dxil > ${VERTEX_DXIL_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${SHADER_DIR_PATH}/${SHADER_NAME}.vert.dxil # clean up generated dxil file
        COMMENT "Generating DXIL Header: ${SHADER} -> ${SHADER_NAME}_vertex_dxil.h"
        DEPENDS "${SHADER_DIR_PATH}/${SHADER}"
    )

    # Generate the dxil fragment shader code
    add_custom_command(
        OUTPUT ${FRAGMENT_DXIL_OUTPUT}
        COMMAND slangc ${SHADER_DIR_PATH}/${SHADER} -target dxil -profile sm_6_0 -stage fragment -entry fragmentMain -o ${SHADER_DIR_PATH}/${SHADER_NAME}.frag.dxil
        COMMAND xxd -i -n "${SHADER_NAME}_fragment_dxil_source" ${SHADER_DIR_PATH}/${SHADER_NAME}.frag.dxil > ${FRAGMENT_DXIL_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E rm -f ${SHADER_DIR_PATH}/${SHADER_NAME}.frag.dxil # clean up generated dxil file
        COMMENT "Generating DXIL Header: ${SHADER} -> ${SHADER_NAME}_fragment_dxil.h"
        DEPENDS "${SHADER_DIR_PATH}/${SHADER}"
    )
endforeach()

add_custom_target(BuiltinShaders ALL DEPENDS ${BUILTIN_SHADER_FILES})