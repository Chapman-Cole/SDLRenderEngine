#version 450

layout(location = 0) in vec3 pos;

layout(location = 0) out vec4 outColor;

layout(std140, set = 3, binding = 0) uniform InputData {
    vec4 color;
    vec2 position;
    vec2 size;
    vec4 cornerRadii;
} Input;

// glslc rectangle.frag -o rectangle.frag.spv
// xxd -i rectangle.frag.spv > RectangleFragmentShader.h
// All together it is:
// glslc rectangle.frag -o rectangle.frag.spv ; xxd -i rectangle.frag.spv > RectangleFragmentShader.h

void main() {
    // Center around zero, since the top left corner is at (0, 0) relative to the input pos
    vec2 adjustedPos = pos.xy;
    adjustedPos.x -= Input.size.x / 2.0f;
    adjustedPos.y += Input.size.y / 2.0f;
    vec2 adjustedBounds = 0.5 * Input.size;

    // Input.cornerRadii.x - Top Left
    // Input.cornerRadii.y - Top Right
    // Input.cornerRadii.z - Bottom Left
    // Input.cornerRadii.w - Bottom Right

    // Top Left Corner
    if (Input.cornerRadii.x > 0.0) {
        float radius = Input.cornerRadii.x;

        float adjustedX = -adjustedPos.x - adjustedBounds.x + radius;
        float root = sqrt(radius * radius - adjustedX * adjustedX);

        if (adjustedPos.x < radius - adjustedBounds.x && adjustedPos.y > adjustedBounds.y - radius + root) {
            discard;
        }
    }

    // Top Right Corner
    if (Input.cornerRadii.y > 0.0) {
        float radius = Input.cornerRadii.y;

        float adjustedX = adjustedPos.x - adjustedBounds.x + radius;
        float root = sqrt(radius * radius - adjustedX * adjustedX);

        if (adjustedPos.x > adjustedBounds.x - radius && adjustedPos.y > adjustedBounds.y - radius + root) {
            discard;
        }
    }

    // Bottom Left Corner
    if (Input.cornerRadii.z > 0.0) {
        float radius = Input.cornerRadii.z;

        float adjustedX = -adjustedPos.x - adjustedBounds.x + radius;
        float root = sqrt(radius * radius - adjustedX * adjustedX);

        if (adjustedPos.x < radius - adjustedBounds.x && adjustedPos.y < -adjustedBounds.y + radius - root) {
            discard;
        }
    }

    // Bottom Right Corner
    if (Input.cornerRadii.w > 0.0) {
        float radius = Input.cornerRadii.w;

        float adjustedX = adjustedPos.x - adjustedBounds.x + radius;
        float root = sqrt(radius * radius - adjustedX * adjustedX);

        if (adjustedPos.x > adjustedBounds.x - radius && adjustedPos.y < -adjustedBounds.y + radius - root) {
            discard;
        }
    }

    outColor = Input.color;
}