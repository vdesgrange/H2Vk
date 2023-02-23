#version 460

const float PI = 3.14159265359;

layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;

layout(set = 0, binding = 2) uniform samplerCube irradianceMap; // aka. environment map
layout(set = 0, binding = 3) uniform samplerCube prefilteredMap; // aka. prefiltered map
layout(set = 0, binding = 4) uniform sampler2D brdfMap; // aka. prefilter map

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

vec3 uncharted_to_tonemap(vec3 x) {
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
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, vec3 albedo, float metallic, float roughness) {
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefiltered_reflection(vec3 R, float roughness) {
    const float MAX_REFLECTION_LOD = 9.0;
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
    vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
    return mix(a, b, lod - lodf);
}

vec3 BRDF(vec3 N, vec3 L, vec3 V, vec3 C, vec3 albedo, float roughness, float metallic) {
    vec3 color = vec3(0.0);

    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);

    if (dotNL > 0.0) {
        float D = D_GGX(dotNH, roughness);
        float G = G_Smith(dotNL, dotNV, roughness);
        vec3 F = F_Schlick(dotHV, albedo, metallic);
        vec3 spec = D * G * F / (4.0 * dotNL * dotNV + 0.001);
        vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);

        color += (kd * albedo / PI + spec) * dotNL;
    }

    return color;
}

vec2 sample_spherical_map(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591f, 0.3183f);
    uv += 0.5;
    return -1 * uv;
}

vec3 calculateNormal() {
    vec3 N = normalize(inNormal);

    vec3 pos_dx = dFdx(inFragPos);
    vec3 pos_dy = dFdy(inFragPos);
    vec3 tex_dx = dFdx(vec3(inUV, 0.0));
    vec3 tex_dy = dFdy(vec3(inUV, 0.0));
    vec3 T = (tex_dy.t * pos_dx - tex_dx.t * pos_dy) / (tex_dx.s * tex_dy.t - tex_dy.s * tex_dx.t);
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    vec3 TN = texture(samplerNormalMap, inUV).xyz;
    TN = normalize(TBN * (2.0 * TN - 1.0));

    return TN;
}


void main()
{
    vec3 albedo = pow(texture(samplerAlbedoMap, inUV).rgb, vec3(2.2));
    // vec3 albedo = texture(samplerAlbedoMap, inUV).rgb; // no gamma correction pow 2.2
    vec3 normal = texture(samplerNormalMap, inUV).rgb;
    float metallic = texture(samplerMetalRoughnessMap, inUV).r;
    float roughness = texture(samplerMetalRoughnessMap, inUV).g;
    float ao = texture(samplerAOMap, inUV).r;
    vec3 emissive = texture(samplerEmissiveMap, inUV).rgb;

    int sources = 1;
    vec3 lightPos = sceneData.sunlightDirection.xyz;
    float lightFactor = sceneData.sunlightDirection.w;

    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 N = calculateNormal(); // normalize(inNormal);
    vec3 C = sceneData.sunlightColor.rgb;
    vec3 R = reflect(-V, N);

    vec2 uv = sample_spherical_map(N);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < sources; i++) {
        vec3 L = normalize(lightPos - inFragPos);
        Lo += BRDF(L, V, N, C, albedo, roughness, metallic);
    };

    vec2 brdf = texture(brdfMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefiltered_reflection(R, roughness).rgb;
    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 diffuse = irradiance * albedo;

    vec3 F = F_SchlickR(max(dot(N, V), 0.0), albedo, metallic, roughness);

    vec3 specular = reflection * (F * brdf.x + brdf.y);

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 ambient = (kD * diffuse + specular) * texture(samplerAOMap, inUV).rrr; // + emissive

    vec3 color = ambient + Lo;

    color = uncharted_to_tonemap(color);
    color = color * (1.0f / uncharted_to_tonemap(vec3(11.2f)));// (color + vec3(1.0)); // Reinhard operator

    outFragColor = vec4(color, 1.0);
}
