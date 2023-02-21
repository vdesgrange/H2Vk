#version 460

layout (binding = 0) uniform samplerCube inputImage;

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConsts {
    layout(offset = 128) float roughness;
} consts;


#define PI 3.1415926535897932384626433832795

vec2 hammersley(uint i, uint N) {
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10; // / 0x100000000

    return vec2(float(i)/float(N), rdi);
}

vec3 importance_sample_GGX(vec2 Xi, vec3 N, float roughness) {
    float alpha = roughness * roughness;

    float phi = 2.0 * PI * Xi.x; // + random(normal.xz) * 0.1
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX   = normalize(cross(up, N));
    vec3 tangentY = cross(N, tangentX);

    vec3 sampleVec = tangentX * H.x + tangentY * H.y + N * H.z;

    return normalize(sampleVec);
}

float D_GGX(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(PI * denom*denom);
}

void main()
{
    vec3 N = normalize(inPos);
    vec3 R = N;
    vec3 V = N;
    float roughness = consts.roughness;
    uint numSamples = 1024u;
    float totalWeight = 0.0;

    vec3 prefiltered = vec3(0.0);
    for(uint i = 0u; i < numSamples; i++) {
        vec2 Xi = hammersley(i, numSamples);
        vec3 H = importance_sample_GGX(Xi, N, roughness);
        vec3 L = 2.0 * dot(V, H) * H - V;
        float dotNL = clamp(dot(N, L), 0.0, 1.0);
        if(dotNL > 0.0) {
            float dotNH = clamp(dot(N, H), 0.0, 1.0);
            float dotVH = clamp(dot(V, H), 0.0, 1.0);

            prefiltered += texture(inputImage, L).rgb * dotNL;

            totalWeight += dotNL;
        }
    }

    prefiltered /= totalWeight;

    outFragColor = vec4(prefiltered, 1.0);
}