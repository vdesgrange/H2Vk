#version 460
//#extension GL_ARB_shading_language_include : require

layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;

layout (push_constant) uniform Material {
    float metallic;
    float roughness;
    float ao;
    vec3 albedo;
} material;

struct ObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData primitives[];
} objectBuffer;

layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265359;

float D_GGX(float dotNH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float dotNH2 = dotNH * dotNH;
    float denom = (dotNH2 * (a2 - 1) + 1);
    denom = PI * denom * denom;

    return a2 / denom;
}

float G_Smith(float dotNV, float dotNL, float roughness) {
    float r = roughness + 1.0;
    float k = r * r / 8;
    float ggx1 = dotNV / (dotNV * (1.0 - k) + k);
    float ggx2 = dotNL / (dotNL * (1.0 - k) + k);

    return ggx1 * ggx2;
}


vec3 F_Schlick(float cosTheta) {
    float albedo;
  vec3 F0 = mix(vec3(0.04), material.albedo, material.metallic);
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 BRDF(vec3 N, vec3 L, vec3 V, vec3 C, float roughness) {
    vec3 color = vec3(0.0);
    float distance = length(L);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = C * attenuation;

    vec3 H = normalize(V + L);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);

    float D = D_GGX(dotNH, roughness);
    vec3 F = F_Schlick(dotNV);
    float G = G_Smith(dotNV, dotNL, roughness);

    vec3 spec = (D * F * G) / (4.0 * dotNL * dotNV + 0.0001);

    vec3 kd = (vec3(1.0) - F) * (1.0 - material.metallic);

    color += spec * radiance * dotNL;
    color += kd * material.albedo / PI * radiance * dotNL;

    return color;
}

void main()
{
    int sources = 1;

    vec3 lightPos = sceneData.sunlightDirection.xyz;

    vec3 C = sceneData.sunlightColor.xyz;
    vec3 N = normalize(inNormal);
    vec3 V = normalize(inCameraPos - inFragPos);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < sources; i++) {
        vec3 L = normalize(lightPos - inFragPos);
        Lo += BRDF(L, V, N, C, material.roughness);
    };

    vec3 ambient = vec3(0.03) * material.albedo;

    vec3 color = ambient + Lo;
    color = pow(color, vec3(0.4545));
    outFragColor = vec4(color, 1.0);
}
