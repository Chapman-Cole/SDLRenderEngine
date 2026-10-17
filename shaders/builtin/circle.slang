struct VertexStageInput {
    uint vertexID : SV_VertexID;
};

struct VertexToFragment {
    float4 sv_position : SV_Position;
    float2 pos : float2;
};

struct FragmentStageOutput {
    float4 color;
};

struct VertexUniformData {
    float2 center;
    float2 windowDims;
    float radius;
    float aspectRatio;
    float2 pad;
};

[[vk::binding(0, 1)]] // SDL3 requires vertex data to be in set 1
ConstantBuffer<VertexUniformData> vertexUniform : register(b0, space1);

[shader("vertex")]
VertexToFragment vertexMain(VertexStageInput vertexInput) {
    static float2 positions[6] = {
        float2(-1.0, -1.0),
        float2(1.0, -1.0),
        float2(-1.0, 1.0),

        float2(1.0, -1.0),
        float2(1.0, 1.0),
        float2(-1.0, 1.0)
    };

    VertexToFragment output;
    output.pos = positions[vertexInput.vertexID];

    float2 tempVec = vertexUniform.radius * positions[vertexInput.vertexID];
    tempVec.x += vertexUniform.center.x;
    tempVec.y -= vertexUniform.center.y;

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
    float2 center;
    float radius;
    float aspectRatio;
};

[[vk::binding(0, 3)]] // SDL3 requires fragment data to be in set 3
ConstantBuffer<FragmentUniformData> fragmentUniform : register(b0, space3);

[shader("fragment")]
FragmentStageOutput fragmentMain(VertexToFragment fragPos) : SV_Target {
    if (fragPos.pos.x * fragPos.pos.x + fragPos.pos.y * fragPos.pos.y > 1.0) {
        discard;
    }

    FragmentStageOutput output;
    output.color = fragmentUniform.color;
    return output;
}

