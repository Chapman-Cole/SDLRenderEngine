#version 450

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),

    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

layout(std140, set = 1, binding = 0) uniform InputStruct {
    vec2 center;
    vec2 size;
    float aspectRatio;
    vec3 pad;
} Input;

layout(location = 0) out vec3 pos;

// glslc rectangle.vert -o rectangle.vert.spv
// xxd -i rectangle.vert.spv > RectangleVertexShader.h
// All together it is:
// glslc rectangle.vert -o rectangle.vert.spv ; xxd -i rectangle.vert.spv > RectangleVertexShader.h

void main() {
    // "0.5 *" because it is scaled in both the negative and positive x directions
    vec2 tempVec = 0.5 * Input.size * positions[gl_VertexIndex];
    tempVec.x *= Input.aspectRatio;

    pos = vec3(tempVec + Input.center, 0.0);
    gl_Position = vec4(pos, 1.0);
}