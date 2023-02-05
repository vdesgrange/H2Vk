#version 460

const float PI = 3.14159265359;

layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D samplerAlbedoMap;
layout(set = 2, binding = 1) uniform sampler2D samplerNormalMap;
layout(set = 2, binding = 2) uniform sampler2D samplerMetalRoughnessMap;
layout(set = 2, binding = 3) uniform sampler2D samplerAOMap;
layout(set = 2, binding = 4) uniform sampler2D samplerEmissiveMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos; // fragment/world position
layout (location = 4) in vec3 inCameraPos; // camera/view position
layout (location = 5) in vec4 inTangent;

layout (location = 0) out vec4 outFragColor;

float D_GGX(float dotNH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2)/(PI * denom*denom);
}

float G_Smith(float dotNV, float dotNL, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float ggx1 = dotNL / (dotNL * (1.0 - k) + k);
    float ggx2 = dotNV / (dotNV * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 F_Schlick(float cosTheta, vec3 albedo, float metallic) {
    vec3 F0 = mix(vec3(0.02), albedo, metallic);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 BRDF(vec3 N, vec3 L, vec3 V, vec3 C, vec3 albedo, float roughness, float metallic) {
    vec3 color = vec3(0.0);

    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);


    float D = D_GGX(dotNH, roughness);
    float G = G_Smith(dotNL, dotNV, roughness);
    vec3 F = F_Schlick(dotHV, albedo, metallic); // dotNV
    vec3 spec = D * G * F / (4.0 * dotNL * dotNV + 0.0001);
    color += spec * C * dotNL;

    vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
    color += kd * albedo / PI * C * dotNL;

    return color;
}

void main()
{
    vec3 albedo = texture(samplerAlbedoMap, inUV).rgb; // no gamma correction pow 2.2
    vec3 normal = texture(samplerNormalMap, inUV).rgb;
    float metallic = texture(samplerMetalRoughnessMap, inUV).r;
    float roughness = texture(samplerMetalRoughnessMap, inUV).g;
    float ao = texture(samplerAOMap, inUV).r;
    vec3 emissive = texture(samplerEmissiveMap, inUV).rgb;

    int sources = 1;
    vec3 lightPos = sceneData.sunlightDirection.xyz;
    float lightFactor = sceneData.sunlightDirection.w;

    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 N = normalize(inNormal);
    vec3 C = sceneData.sunlightColor.rgb;

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < sources; i++) {
        vec3 L = normalize(lightPos - inFragPos);
        Lo += BRDF(L, V, N, C, albedo, roughness, metallic);
    };

    vec3 color = Lo;
    color += vec3(0.03) * albedo * ao;
    color += emissive;

    color = color / (color + vec3(1.0)); // Reinhard operator
    outFragColor = vec4(color, albedo);
}
