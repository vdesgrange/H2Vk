#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../common/constants.glsl"
#include "../common/brdf.glsl"
#include "../common/filters.glsl"
#include "../common/debug.glsl"

layout (std140, set = 0, binding = 0) uniform EnabledFeaturesData {
    bool shadowMapping;
    bool skybox;
    bool atmosphere;
} enabledFeaturesData;


layout (std140, set = 0, binding = 3) uniform ShadowData {
    layout(offset = 0) mat4 cascadeVP[CASCADE_COUNT];
    layout(offset = 256) vec4 splitDepth;
    layout(offset = 272) bool color_cascades;
} depthData;

layout(std140, set = 0, binding = 2) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

//layout (std140, set = 0, binding = 3) uniform ShadowData {
//    layout(offset = 0) vec2  num_lights;
//    layout(offset = 16) mat4 directional_mvp[MAX_LIGHT];
//    layout(offset = 528) mat4 spot_mvp[MAX_LIGHT];
//} depthData;


layout(set = 0, binding = 4) uniform samplerCube irradianceMap; // aka. environment map
layout(set = 0, binding = 5) uniform samplerCube prefilteredMap; // aka. prefiltered map
layout(set = 0, binding = 6) uniform sampler2D brdfMap; // aka. prefilter map
layout(set = 0, binding = 7) uniform sampler2DArray shadowMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;
layout (location = 5) in vec3 inViewPos;
layout (location = 6) in vec4 inTangent;

layout (push_constant) uniform Material {
    layout(offset = 64) vec4 albedo; // update offset for csm
    layout(offset = 80) float metallic;
    layout(offset = 84) float roughness;
    layout(offset = 88) float ao;
} material;

layout (location = 0) out vec4 outFragColor;


//vec3 shadow(vec3 color, int type) {
//    float layer_offset = 0;
//    vec4 shadowCoord;
//
//    if (type == 1) {
//        layer_offset = depthData.num_lights[0];
//    }
//
//    for (int i=0; i < depthData.num_lights[type]; i++) {
//        if (type == 0) {
//            shadowCoord = biasMat * depthData.spot_mvp[i] * vec4(inFragPos, 1.0);
//        } else if (type == 1) {
//            shadowCoord = biasMat * depthData.directional_mvp[i] * vec4(inFragPos, 1.0);
//        }
//
//        float shadowFactor = (enablePCF == 1) ? filterPCF(shadowMap, inNormal, inFragPos, shadowCoord, layer_offset + i) : texture_projection(shadowMap,  inNormal, inFragPos, shadowCoord, vec2(0.0), layer_offset + i);
//        color *= shadowFactor;
//    }
//    return color;
//}

vec3 prefiltered_reflection(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 9.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
    vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}

vec3 BRDF(vec3 N, vec3 L, vec3 V, vec3 C) {
    vec3 color = vec3(0.0);
    float roughness = material.roughness;
    vec3 F0 = mix(vec3(0.02), material.albedo.rgb, material.metallic);


    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);

    if (dotNL > 0.0)
    {
        float D = D_GGX(dotNH, roughness * roughness);
        float G = G_GGX(dotNL, dotNV, roughness);
        vec3 F = F_Schlick(F0, dotHV); // dotNV
        vec3 spec = D * G * F / (4.0 * dotNL * dotNV + 0.001);
        vec3 kd = (vec3(1.0) - F) * (1.0 - material.metallic);
        color += (kd * material.albedo.rgb / PI + spec) * dotNL;
    }

    return color;
}

vec2 sample_spherical_map(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591f, 0.3183f);
    uv += 0.5;
    return uv;
}

vec3 spot_light(vec3 Lo, vec3 N, vec3 V) {
    for (int i = 0; i < lightingData.num_lights[0]; i++) {
        vec3 L = normalize(lightingData.spot_position[i].xyz - inFragPos);
        vec3 C = lightingData.spot_color[i].rgb;

        Lo += BRDF(L, V, N, C);
    };

    return Lo;
}

vec3 directional_light(vec3 Lo, vec3 N, vec3 V) {
    for (int i = 0; i < lightingData.num_lights[1]; i++) {
        vec3 L = normalize(lightingData.dir_direction[i].xyz);
        vec3 C = lightingData.dir_color[i].rgb;

        Lo += BRDF(L, V, N, C);
    };

    return Lo;
}

void main()
{
    float roughness = material.roughness;
    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 N = normalize(inNormal);
    vec3 R = reflect(-V, N);

    vec2 uv = sample_spherical_map(N);
    vec3 A = texture(irradianceMap, N).rgb;

    vec3 Lo = vec3(0.0);
    // Lo = spot_light(Lo, N, V);
    Lo = directional_light(Lo, N, V);

    vec2 brdf = texture(brdfMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefiltered_reflection(R, roughness).rgb;
    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 diffuse = irradiance *  material.albedo.rgb;

    vec3 F0 = mix(vec3(0.02), material.albedo.rgb, material.metallic);
    vec3 F = F_SchlickR(F0, max(dot(N, V), 0.0), roughness);

    vec3 specular = reflection * (F * brdf.x + brdf.y);

    vec3 kD = (vec3(1.0) - F) * (1.0 - material.metallic);
    vec3 ambient = (kD * diffuse + specular);

    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0)); // Reinhard operator
//    color = shadow(color, 0);
//    color = shadow(color, 1);

    // Convert from linear to sRGB ! Do not use for Vulkan !
    // color = pow(color, vec3(0.4545)); // Gamma correction

    if (enabledFeaturesData.shadowMapping == true) {
        // Cascaded shadow mapping
        uint cascadeIndex = 0;
        for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
            if(inViewPos.z < depthData.splitDepth[i]) {
                cascadeIndex = i + 1;
            }
        }

        vec4 coord = biasMat * depthData.cascadeVP[cascadeIndex] * vec4(inFragPos, 1.0);
        vec3 L = normalize(lightingData.dir_direction[0].xyz);
        // float shadow = texture_projection(shadowMap, N, L, coord / coord.w, vec2(0.0), cascadeIndex);
        float shadow = filterSPS(shadowMap, N, L, coord / coord.w, cascadeIndex);
        color.rgb *= shadow;

        debug_cascades(depthData.color_cascades, cascadeIndex, color);
    }


    outFragColor = vec4(color, 1.0);
}
