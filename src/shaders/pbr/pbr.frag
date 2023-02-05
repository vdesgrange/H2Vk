#version 460
// #extension GL_ARB_shading_language_include : require
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
    vec4 albedo; // w for opacity
    float metallic;
    float roughness;
    float ao;
} material;

layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265359;

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

vec3 F_Schlick(float cosTheta) {
    vec3 F0 = mix(vec3(0.02), material.albedo.rgb, material.metallic);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 BRDF(vec3 N, vec3 L, vec3 V, vec3 C) {
    vec3 color = vec3(0.0);
    float roughness = material.roughness;

    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);

    if (dotNL > 0.0)
    {
        float D = D_GGX(dotNH, roughness);
        float G = G_Smith(dotNL, dotNV, roughness);
        vec3 F = F_Schlick(dotHV); // dotNV
        vec3 spec = D * G * F / (4.0 * dotNL * dotNV + 0.0001);
        color += spec * C * dotNL;

        vec3 kd = (vec3(1.0) - F) * (1.0 - material.metallic);
        color += kd * material.albedo.rgb / PI * C * dotNL;
    }

    return color;
}

void main()
{
    int sources = 1;

    vec3 lightPos = sceneData.sunlightDirection.xyz;
    float lightFactor = sceneData.sunlightDirection.w;

    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 N = normalize(inNormal);
    vec3 C = sceneData.sunlightColor.rgb;

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < sources; i++) {
        vec3 L = normalize(lightPos - inFragPos);
//        float distance = length(lightPos - inFragPos);
//        float attenuation = 1.0 / (distance * distance);
//        vec3 radiance = C * attenuation;

        Lo += BRDF(L, V, N, C);

        vec3 ambient = vec3(0.03) * material.albedo.xyz * material.ao * vec3(dot(N, L) / sources); // lightFactor
        Lo += ambient;
    };

    // vec3 ambient = 0.02 * material.albedo.xyz * material.ao;
    vec3 color = Lo; // ambient + Lo;
    color = color / (color + vec3(1.0)); // Reinhard operator

    // Convert from linear to sRGB ! Do not use for Vulkan !
    // color = pow(color, vec3(0.4545)); // Gamma correction

    outFragColor = vec4(color, material.albedo.w);
}