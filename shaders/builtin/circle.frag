#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec4 outColor;

layout(std140, set = 3, binding = 0) uniform InputData {
    vec4 color;
    vec2 center;
    float radius;
    float aspectRatio;
} Input;

void main() {
    vec2 newPos = pos.xy - Input.center;
    newPos.x *= 1.0 / Input.aspectRatio;
    if (distance(newPos, vec2(0, 0)) > Input.radius) {
        discard;
    }

    outColor = Input.color;
}