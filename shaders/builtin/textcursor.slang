struct VertexStageInput {
    uint vertexID : SV_VertexID;
};

struct VertexToFragment {
    float4 sv_position : SV_Position;
};

struct FragmentStageOutput {
    float4 color;
}

struct VertexUniformData {
    float2 position;
    float2 size;
    float2 windowDims;
    float2 pad;
}

[[vk::binding(0,1)]] // SDL3 requires vertex uniform data to be in set 1
ConstantBuffer<VertexUniformData> vertexUniform : register(b0, space1); 

[shader("vertex")]
VertexToFragment vertexMain(VertexStageInput vertexInput) {
    // If wondering why rectangle has (0,0) as top left corner, see spiel in rectangle.slang
    static float2 positions[6] = {
        float2(0.0, 0.0),
        float2(0.0, -1.0),
        float2(1.0, 0.0),

        float2(1.0, 0.0),
        float2(0.0, -1.0),
        float2(1.0, -1.0)
    };

    VertexToFragment output;

    float2 tempVec = positions[vertexInput.vertexID];

    tempVec.x *= vertexUniform.size.x;
    tempVec.y *= vertexUniform.size.y;

    tempVec.x += vertexUniform.position.x;
    tempVec.y -= vertexUniform.position.y;

    float pixelScaleX = (1.0 - (-1.0)) / vertexUniform.windowDims.x;
    float pixelScaleY = (1.0 - (-1.0)) / vertexUniform.windowDims.y;

    tempVec.x *= pixelScaleX;
    tempVec.y *= pixelScaleY;

    // Treat things as relative to the top left corner
    tempVec.x -= 1;
    tempVec.y += 1;

    output.sv_position = float4(tempVec, 0.0, 1.0);
    return output;
}

struct FragmentUniformData {
    float4 color;
}

[[vk::binding(0,3)]] // SDL3 requires fragment uniform data to be in set 3
ConstantBuffer<FragmentUniformData> fragmentUniform : register(b0, space3);

[shader("fragment")]
FragmentStageOutput fragmentMain(VertexToFragment fragmentInput) : SV_Target {
    FragmentStageOutput output;
    output.color = fragmentUniform.color;
    return output;
}