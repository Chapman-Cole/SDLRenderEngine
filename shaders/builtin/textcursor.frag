#version 450

layout(location = 0) out vec4 FragColor;

layout(std140, set = 3, binding = 0) uniform InputStruct {
    vec4 color;
} Input;

void main() {
    FragColor = Input.color;
}