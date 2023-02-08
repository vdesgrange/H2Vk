#version 460
// #extension GL_ARB_shading_language_include : require
layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;

layout(set = 0, binding = 2) uniform sampler2D irradianceMap; // aka. environment map
layout(set = 0, binding = 3) uniform sampler2D prefilteredMap; // radiance map?
layout(set = 0, binding = 4) uniform sampler2D brdfMap; // radiance map?

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;

layout (push_constant) uniform Material {
    layout(offset = 64) vec4 albedo;
    layout(offset = 80) float metallic;
    layout(offset = 84) float roughness;
    layout(offset = 88) float ao;
} material;

layout (location = 0) out vec4 outFragColor;

const float PI = 3.14159265359;

vec3 uncharted_2_tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}


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


vec3 F_SchlickR(float cosTheta, vec3 albedo, float metallic, float roughness) {
    vec3 F0 = mix(vec3(0.02), albedo, metallic);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefiltered_reflection(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 3.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
    vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}

vec3 specular_contribution(vec3 N, vec3 L, vec3 V, vec3 C, vec3 albedo, float roughness, float metallic) {
    vec3 color = vec3(0.0);

    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);


    float D = D_GGX(dotNH, roughness);
    float G = G_Smith(dotNL, dotNV, roughness);
    vec3 F = F_Schlick(dotHV, albedo, metallic);
    vec3 spec = D * G * F / (4.0 * dotNL * dotNV + 0.0001);
    color += spec * C * dotNL;

    vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
    color += kd * albedo / PI * C * dotNL;

    return color;
}

vec2 sample_spherical_map(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591f, 0.3183f);
    uv += 0.5;
    return -1 * uv;
}

void main()
{
    int sources = 1;
    vec3 lightPos = sceneData.sunlightDirection.xyz;
    float lightFactor = sceneData.sunlightDirection.w;

    float roughness = material.roughness;
    vec3 albedo = material.albedo;
    float metallic = material.metallic;

    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 N = normalize(inNormal);
    vec3 C = sceneData.sunlightColor.rgb;
    vec2 uv = sample_spherical_map(N);
    vec3 A = texture(irradianceMap, uv).rgb; // vec3(0.03)

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < sources; i++) {
        vec3 L = normalize(lightPos - inFragPos);
        Lo += specular_contribution(L, V, N, C, albedo, roughness, metallic);
    };

    vec2 brdf = texture(brdfMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefiltered_reflection(R, roughness).rgb;
    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 diffuse = irradiance * albedo;

    vec3 F = F_SchlickR(max(dot(N, V), 0.0), roughness);

    vec3 specular = reflection * (F * brdf.x + brdf.y);

    vec3 kd = 1.0 - F;
    kd *= 1.0 - metallic;
    vec3 ambient = (kd * diffuse + specular);
    vec3 color = ambient + Lo;

    // color = uncharted_2_tonemap(color * uboParams.exposure);
    // color = color * (1.0f / uncharted_2_tonemap(vec3(11.2f)));
    // color = color / (color + vec3(1.0)); // Reinhard operator
    // Convert from linear to sRGB ! Do not use for Vulkan !
    // color = pow(color, vec3(0.4545)); // Gamma correction

    outFragColor = vec4(color, material.albedo.w);
}
