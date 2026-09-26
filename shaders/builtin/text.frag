#version 450

layout(location = 0) in vec2 pos;

layout(location = 1) in vec2 tex;

layout(set = 2, binding = 0) uniform sampler2D atlas;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 1.0, 1.0, texture(atlas, tex).a);
}