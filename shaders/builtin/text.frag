#version 450

layout(location = 0) in vec2 tex;

layout(set = 2, binding = 0) uniform sampler2D atlas;

layout(location = 0) out vec4 outColor;

layout(std140, set = 3, binding = 0) uniform InputData {
    vec4 color;
} Input;

void main() {
    outColor = vec4(Input.color.rgb, min(Input.color.a, texture(atlas, tex).a));
}