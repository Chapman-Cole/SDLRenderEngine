struct VertexStageInput {
    uint vertexID : SV_VertexID;
};

struct VertexToFragment {
    float2 pos : float2;
    float4 sv_position : SV_Position;
};

struct FragmentStageOutput {
    float4 color;
};

struct VertexUniformData {
    float2 position;
    float2 size;
    float2 windowDims;
    float2 pad;
};

[[vk::binding(0, 1)]] // SDL3 requires vertex uniform data to be in set 1
ConstantBuffer<VertexUniformData> vertexUniform : register(b0, space1);

[shader("vertex")]
VertexToFragment vertexMain(VertexStageInput vertexInput) {
    static float2 positions[6] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(0.0, 1.0),

        float2(1.0, 0.0),
        float2(1.0, 1.0),
        float2(0.0, 1.0)
    };

    VertexToFragment output;

    float2 tempVec = positions[vertexInput.vertexID];
    tempVec.x *= vertexUniform.size.x;
    tempVec.y *= -vertexUniform.size.y;
    output.pos = tempVec;

    tempVec.x += vertexUniform.position.x;
    tempVec.y -= vertexUniform.position.y;

    float pixelScaleX = (1.0 - (-1.0)) / vertexUniform.windowDims.x;
    float pixelScaleY = (1.0 - (-1.0)) / vertexUniform.windowDims.y;

    tempVec.x *= pixelScaleX;
    tempVec.y *= pixelScaleY;

    // Translates to upper left corner
    tempVec.x -= 1;
    tempVec.y += 1;

    output.sv_position = float4(tempVec, 0.0, 1.0);

    return output;
}

struct FragmentUniformData {
    float4 color;
    float2 position;
    float2 size;
    float4 cornerRadii;
};

[[vk::binding(0, 3)]] // SDL3 requires fragment uniform data to be in set 3
ConstantBuffer<FragmentUniformData> fragmentUniform : register(b0, space3);

[shader("fragment")]
FragmentStageOutput fragmentMain(VertexToFragment fragPos) : SV_Target {
    float2 adjustedPos = fragPos.pos;
    adjustedPos.x -= fragmentUniform.size.x / 2.0;
    adjustedPos.y += fragmentUniform.size.y / 2.0;
    float2 adjustedBounds = 0.5 * fragmentUniform.size;

    // Input.cornerRadii.x - Top Left
    // Input.cornerRadii.y - Top Right
    // Input.cornerRadii.z - Bottom Left
    // Input.cornerRadii.w - Bottom Right

    // Top Left Corner
    if (fragmentUniform.cornerRadii.x > 0.0) {
        float radius = fragmentUniform.cornerRadii.x;

        float adjustedX = -adjustedPos.x - adjustedBounds.x + radius;

        if (adjustedPos.x < radius - adjustedBounds.x) {
            float root = sqrt(radius * radius - adjustedX * adjustedX);

            if (adjustedPos.y > adjustedBounds.y - radius + root) {
                discard;
            }
        }
    }

    // Top Right Corner
    if (fragmentUniform.cornerRadii.y > 0.0) {
        float radius = fragmentUniform.cornerRadii.y;

        float adjustedX = adjustedPos.x - adjustedBounds.x + radius;

        if (adjustedPos.x > adjustedBounds.x - radius) {
            float root = sqrt(radius * radius - adjustedX * adjustedX);

            if (adjustedPos.y > adjustedBounds.y - radius + root) {
                discard;
            }
        }
    }

    // Bottom Left Corner
    if (fragmentUniform.cornerRadii.z > 0.0) {
        float radius = fragmentUniform.cornerRadii.z;

        float adjustedX = -adjustedPos.x - adjustedBounds.x + radius;

        if (adjustedPos.x < radius - adjustedBounds.x) {
            float root = sqrt(radius * radius - adjustedX * adjustedX);

            if (adjustedPos.y < -adjustedBounds.y + radius - root) {
                discard;
            }
        }
    }

    // Bottom Right Corner
    if (fragmentUniform.cornerRadii.w > 0.0) {
        float radius = fragmentUniform.cornerRadii.w;

        float adjustedX = adjustedPos.x - adjustedBounds.x + radius;

        if (adjustedPos.x > adjustedBounds.x - radius) {
            float root = sqrt(radius * radius - adjustedX * adjustedX);

            if (adjustedPos.y < -adjustedBounds.y + radius - root) {
                discard;
            }
        }
    }

    FragmentStageOutput output;
    output.color = fragmentUniform.color;
    return output;
}