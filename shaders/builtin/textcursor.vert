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

void main() {
    // Top left is treated as origin and down as positive, while the vertex coordinates use cartesian system,
    // requiring the akward negative signs for the y values below since up is positive in cartesian system
    vec2 tempVec = positions[gl_VertexIndex];

    tempVec.x *= Input.size.x;
    tempVec.y *= -Input.size.y;

    tempVec.x += Input.position.x;
    tempVec.y -= Input.position.y;

    float pixelScaleX = (1.0 - (-1.0)) / Input.windowDims.x;
    float pixelScaleY = (1.0 - (-1.0)) / Input.windowDims.y;

    tempVec.x *= pixelScaleX;
    tempVec.y *= pixelScaleY;

    // Treat things as relative to the top left corner
    tempVec.x -= 1;
    tempVec.y += 1;

    gl_Position = vec4(tempVec, 0.0, 1.0);
}