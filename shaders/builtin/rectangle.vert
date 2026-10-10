#version 450

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),

    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

layout(std140, set = 1, binding = 0) uniform InputStruct {
    vec2 position;
    vec2 size;
    vec2 windowDims;
    vec2 pad;
} Input;

layout(location = 0) out vec3 pos;

// glslc rectangle.vert -o rectangle.vert.spv
// xxd -i rectangle.vert.spv > RectangleVertexShader.h
// All together it is:
// glslc rectangle.vert -o rectangle.vert.spv ; xxd -i rectangle.vert.spv > RectangleVertexShader.h

void main() {
    // "0.5 *" because it is scaled in both the negative and positive x directions
    vec2 tempVec = positions[gl_VertexIndex];
    tempVec.x *= Input.size.x;
    tempVec.y *= -Input.size.y;
    pos = vec3(tempVec, 0.0);

    tempVec.x += Input.position.x;
    tempVec.y -= Input.position.y;

    float pixelScaleX = (1.0 - (-1.0)) / Input.windowDims.x;
    float pixelScaleY = (1.0 - (-1.0)) / Input.windowDims.y;

    tempVec.x *= pixelScaleX;
    tempVec.y *= pixelScaleY;

    // Translate to upper left corner
    tempVec.x -= 1;
    tempVec.y += 1;

    gl_Position = vec4(tempVec, 0.0, 1.0);
}