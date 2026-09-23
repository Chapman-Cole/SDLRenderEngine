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
    float radius;
    float aspectRatio;
} Input;

layout(location = 0) out vec3 pos;

void main() {
    vec2 tempVec = Input.radius * positions[gl_VertexIndex];
    pos = vec3(tempVec.x * Input.aspectRatio, tempVec.y, 0.0);
    pos.xy += Input.center;
    gl_Position = vec4(pos, 1.0);
}