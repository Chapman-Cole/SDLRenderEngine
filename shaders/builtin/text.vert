#version 450

layout(location = 0) in vec2 pos;

layout(location = 1) in vec2 tex;

layout(location = 0) out vec2 fragPos;

layout(location = 1) out vec2 fragTex;

layout(std140, set = 1, binding = 0) uniform InputData {
    vec2 size;
    vec2 position;
    vec2 windowDims;
    float aspectRatio;
    float displayScale;
    vec2 offset;
    vec2 pad;
} Input;

void main() {
    vec2 tempPos = pos - Input.position + Input.offset;

    float pixelScaleX = (1.0 - (-1.0)) / Input.windowDims.x;
    float pixelScaleY = (1.0 - (-1.0)) / Input.windowDims.y;

    tempPos.x *= pixelScaleX;
    tempPos.y *= pixelScaleY;

    tempPos.x -= 1;
    tempPos.y += 1;
    gl_Position = vec4(tempPos, 0.0, 1.0);  
    fragPos = pos;
    fragTex = tex;
}