#version 460
#extension GL_EXT_debug_printf : disable
#extension GL_GOOGLE_include_directive : enable

#include "../common/brdf.glsl"

layout (set = 0, binding = 0) uniform samplerCube inputImage;

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConsts {
    layout(offset = 128) float roughness;
} consts;

/**
* Prefilter environment map
* Shape of distribution changed based on viewing angle.
* Approximation assume the angle is 0 : n = v = r
* [Real Shading in Unreal Engine 4 - Epic Games]
* [Chapter 20. GPU-Based Importance Sampling - GPU Gems 3]
*/
vec3 prefilter(vec3 N, float roughness, float resolution) {
    vec3 V = N; // n = v = r
    vec3 prefiltered = vec3(0.0);

    float totalWeight = 0.0;
    uint numSamples = 1024u;

    for(uint i = 0u; i < numSamples; i++) {
        vec2 Xi = hammersley(i, numSamples);
        vec3 H = importance_sample_GGX(N, Xi, roughness);
        vec3 L = 2.0 * dot(V, H) * H - V;
        float dotNL = clamp(dot(N, L), 0.0, 1.0);

        // Sample in upper atmosphere
        if(dotNL > 0.0) {
            float dotNH = clamp(dot(N, H), 0.0, 1.0);
            float dotVH = clamp(dot(V, H), 0.0, 1.0);

            float K = 1.0f; // other version : 4.0f;
            float D = D_GGX(dotNH, roughness * roughness);
            float pdf = D * dotNH / (4.0 * dotVH) + 0.0001; // other version : dotNL / PI;
            // Omega S - Solid angle of current sample
            float S = 1.0 / (float(numSamples) * pdf); // other version : 1.0 / pdf;
            // Omega P - Solid angle of current pixel
            float P  = 4.0 * PI / (6.0 * resolution * resolution);

            float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(K * S / P), 0.0f);

            prefiltered += textureLod(inputImage, L, mipLevel).rgb * dotNL;

            totalWeight += dotNL;
        }
    }

    prefiltered /= max(totalWeight, 0.001f);

    return prefiltered;
}


void main()
{
    vec3 N = normalize(inPos);
    float resolution = float(textureSize(inputImage, 0)[0]);
    float roughness = consts.roughness;

    vec3 prefiltered = prefilter(N, roughness, resolution);

    outFragColor = vec4(prefiltered, 1.0);
}