#version 450

#define PI 3.14159265359
#define HALF_PI 1.57079632679
#define TAU 6.28318530718

layout(location = 0) in vec3 pos;

layout(location = 0) out vec4 FragColor;

// Random function from unity docs
float random_num(vec2 uv) {
    return fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123);
}

vec2 random_unit_vec(vec2 seed) {
    float angle = TAU * random_num(seed);
    return vec2(cos(angle), sin(angle));
}

float smoothstep(float x) {
    return x * x * (3.0 - 2.0 * x);
}

float perlinNoise(vec2 seed) {
    vec2 seedClamped = floor(seed);

    vec2 blRand = random_unit_vec(seedClamped);
    float bl = dot(blRand, seed - seedClamped);

    vec2 brRand = random_unit_vec(seedClamped + vec2(1.0, 0.0));
    float br = dot(brRand, seed - (seedClamped + vec2(1.0, 0.0)));

    vec2 tlRand = random_unit_vec(seedClamped + vec2(0.0, 1.0));
    float tl = dot(tlRand, seed - (seedClamped + vec2(0.0, 1.0)));

    vec2 trRand = random_unit_vec(seedClamped + vec2(1.0, 1.0));
    float tr = dot(trRand, seed - (seedClamped + vec2(1.0, 1.0)));

    float horizontal = bl + (smoothstep(seed.x - seedClamped.x) * (br - bl));
    float vertical = tl + (smoothstep(seed.x - seedClamped.x) * (tr - tl));

    return horizontal + smoothstep(seed.y - seedClamped.y) * (vertical - horizontal);
}

void main() {
    const float scale = 10.0;

    float perlin = 0.5 + 0.5 * perlinNoise(scale * pos.xy);

    FragColor = vec4(perlin * vec3(1.0, 1.0, 1.0), 1.0);
}