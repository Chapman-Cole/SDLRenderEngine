struct VertexStageInput {
    [[vk::location(0)]] float2 pos : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
};

struct VertexToFragment {
    float2 fragTex : float2;
    float4 sv_position : SV_Position;
};

struct FragmentStageOutput {
    float4 color;
};

struct VertexUniformData {
    float2 size;
    float2 position;
    float2 windowDims;
    float2 pad;
};

[[vk::binding(0,1)]] // SDL3 requires vertex uniform data to be in set 1
ConstantBuffer<VertexUniformData> vertexUniform : register(b0, space1);

[shader("vertex")]
VertexToFragment vertexMain(VertexStageInput vertexInput) {
    float2 tempPos = vertexInput.pos;
    tempPos.x += vertexUniform.position.x;
    // It is negative because the offset and position are relative to the top left corner (down is treated as positive),
    // while the vertex coordinates use the standard cartesian system where up is positive
    tempPos.y += -vertexUniform.position.y;

    float pixelScaleX = (1.0 - (-1.0)) / vertexUniform.windowDims.x;
    float pixelScaleY = (1.0 - (-1.0)) / vertexUniform.windowDims.y;

    tempPos.x *= pixelScaleX;
    tempPos.y *= pixelScaleY;

    tempPos.x -= 1;
    tempPos.y += 1;

    VertexToFragment output;
    output.sv_position = float4(tempPos, 0.0, 1.0);
    output.fragTex = vertexInput.uv;
    return output;
}

struct FragmentUniformData {
    float4 color;
};

[[vk::binding(0, 3)]] // SDL3 requires fragment uniform data to be in set 3
ConstantBuffer<FragmentUniformData> fragmentUniform : register(b0, space3);

[[vk::binding(0, 2)]] // SDL3 requires fragment sampler data to be in set 2
Sampler2D atlas : register(t0, space2) : register(s0, space2); // The registers are for compatibility with d3d12, which technically requires separate texture2D and SamplerState objects



[shader("fragment")]
FragmentStageOutput fragmentMain(VertexToFragment fragmentInput) : SV_Target {
    FragmentStageOutput output;
    output.color = float4(fragmentUniform.color.rgb, min(fragmentUniform.color.a, atlas.Sample(fragmentInput.fragTex)).a);
    return output;
}