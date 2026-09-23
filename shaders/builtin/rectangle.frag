#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec4 outColor;

layout(std140, set = 3, binding = 0) uniform InputData {
    vec4 color;
    vec2 center;
    vec2 size;
    float roundness;
    float aspectRatio;
    vec2 pad;
} Input;

// glslc rectangle.frag -o rectangle.frag.spv
// xxd -i rectangle.frag.spv > RectangleFragmentShader.h
// All together it is:
// glslc rectangle.frag -o rectangle.frag.spv ; xxd -i rectangle.frag.spv > RectangleFragmentShader.h

void main() {
    vec2 adjustedPos = pos.xy - Input.center;
    adjustedPos.x *= 1.0 / Input.aspectRatio; // Undo the aspect ratio transformation to simplify the math

    // times 0.5 so that it can't extend beyond the halfway point for the curvature
    float radius = Input.roundness * min(Input.size.x, Input.size.y) * 0.5;

    vec2 adjustedBounds = 0.5 * Input.size;

    float adjustedX = adjustedPos.x - adjustedBounds.x + radius;
    float root1 = sqrt(radius * radius - adjustedX * adjustedX);

    float adjustedX2 = -adjustedPos.x - adjustedBounds.x + radius;
    float root2 = sqrt(radius * radius - adjustedX2 * adjustedX2);

    if (Input.roundness > 0.0) {
        if (
            (adjustedPos.x > adjustedBounds.x - radius && adjustedPos.y > adjustedBounds.y - radius + root1) ||
            (adjustedPos.x > adjustedBounds.x - radius && adjustedPos.y < -adjustedBounds.y + radius - root1) || 
            (adjustedPos.x < radius - adjustedBounds.x && adjustedPos.y > adjustedBounds.y - radius + root2) ||
            (adjustedPos.x < radius - adjustedBounds.x && adjustedPos.y < -adjustedBounds.y + radius - root2)
            ) {
            discard;
        }
    }

    outColor = Input.color;
}