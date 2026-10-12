struct VertexStageInput {
    uint vertexID : SV_VertexID;
};

struct VertexToFragment {
    float3 pos : float3;
    float4 sv_position : SV_Position;
};

struct FragmentStageOutput {
    float4 color;
};

struct VertexUniformData {
    float2 center;
    float radius;
    float aspectRatio;
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
        float2(-1.0, 1.0),
        float2(1.0, 1.0)
    };

    VertexToFragment output;

    float2 tempVec = vertexUniform.radius * positions[vertexInput.vertexID];
    output.pos = float3(tempVec.x * vertexUniform.aspectRatio, tempVec.y, 0.0);
    output.pos.xy += vertexUniform.center;
    output.sv_position = float4(output.pos, 1.0);
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
    float2 newPos = fragPos.pos.xy - fragmentUniform.center;
    newPos.x *= 1.0 / fragmentUniform.aspectRatio;
    if (distance(newPos, float2(0, 0)) > fragmentUniform.radius) {
        discard;
    }

    FragmentStageOutput output;
    output.color = fragmentUniform.color;
    return output;
}

