#define PI 3.14159265359
#define HALF_PI 1.57079632679
#define TAU 6.28318530718

// Random function from unity docs
float random_num(float2 uv) {
    return fract(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123);
}

float2 random_unit_vec(float2 seed) {
    float angle = TAU * random_num(seed);
    return float2(cos(angle), sin(angle));
}

float smoothstep(float x) {
    return x * x * (3.0 - 2.0 * x);
}

float perlinNoise(float2 seed) {
    float2 seedClamped = floor(seed);

    float2 blRand = random_unit_vec(seedClamped);
    float bl = dot(blRand, seed - seedClamped);

    float2 brRand = random_unit_vec(seedClamped + float2(1.0, 0.0));
    float br = dot(brRand, seed - (seedClamped + float2(1.0, 0.0)));

    float2 tlRand = random_unit_vec(seedClamped + float2(0.0, 1.0));
    float tl = dot(tlRand, seed - (seedClamped + float2(0.0, 1.0)));

    float2 trRand = random_unit_vec(seedClamped + float2(1.0, 1.0));
    float tr = dot(trRand, seed - (seedClamped + float2(1.0, 1.0)));

    float horizontal = bl + (smoothstep(seed.x - seedClamped.x) * (br - bl));
    float vertical = tl + (smoothstep(seed.x - seedClamped.x) * (tr - tl));

    return horizontal + smoothstep(seed.y - seedClamped.y) * (vertical - horizontal);
}

struct VertexStageInput {
    uint vertexID : SV_VertexID;
};

struct Vertex2Fragment {
    float2 pos : float2;
    float4 sv_position : SV_Position;
};

struct FragmentStageOutput {
    float4 color;
};

[shader("vertex")]
Vertex2Fragment vertexMain(VertexStageInput vertexInput) {
    static float2 positions[6] = {
        float2(-1.0, -1.0),
        float2(1.0, -1.0),
        float2(-1.0, 1.0),

        float2(1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, 1.0)
    };

    Vertex2Fragment output;
    output.sv_position = float4(positions[vertexInput.vertexID], 0.0, 1.0);
    output.pos = output.sv_position.xy;

    return output;
}

[shader("fragment")]
FragmentStageOutput fragmentMain(Vertex2Fragment fragInput) : SV_Target {
    FragmentStageOutput output;

    const float scale = 10.0;

    float perlin = 0.5 + 0.5 * perlinNoise(scale * fragInput.pos);

    output.color = float4(perlin * float3(1.0, 1.0, 1.0), 1.0);
    return output;
}