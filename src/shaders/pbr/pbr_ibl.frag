#version 460
// #extension GL_EXT_debug_printf : disable
// #extension GL_ARB_shading_language_include : require
#define shadow_ambient 0.1

const float PI = 3.14159265359;
const int MAX_LIGHT = 8;
const int enablePCF = 0;

layout(std140, set = 0, binding = 1) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

layout (std140, set = 0, binding = 2) uniform ShadowData {
    layout(offset = 0) vec2  num_lights;
    layout(offset = 16) mat4 directional_mvp[MAX_LIGHT];
    layout(offset = 528) mat4 spot_mvp[MAX_LIGHT];
} depthData;


layout(set = 0, binding = 3) uniform samplerCube irradianceMap; // aka. environment map
layout(set = 0, binding = 4) uniform samplerCube prefilteredMap; // aka. prefiltered map
layout(set = 0, binding = 5) uniform sampler2D brdfMap; // aka. prefilter map
layout(set = 0, binding = 6) uniform sampler2DArray shadowMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;
layout (location = 5) in vec4 inTangent;

layout (push_constant) uniform Material {
    layout(offset = 64) vec4 albedo; // w for opacity
    layout(offset = 80) float metallic;
    layout(offset = 84) float roughness;
    layout(offset = 88) float ao;
} material;

layout (location = 0) out vec4 outFragColor;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );

float textureProj(vec4 P, vec2 off, float layer) {
    vec4 shadowCoord = P / P.w; //  w = 1 for orthographic. Divide by w to emulate perspective.
    float cosTheta = clamp(dot(inNormal, -inFragPos), 0.0, 1.0);
    float bias = 0.005 * tan(acos(cosTheta));

    bias = clamp(bias, 0, 0.01);

    float shadow = 1.0; // default coefficient, bias handled outside.
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) // depth in valid [-1, 1] interval
    {
        float dist = texture(shadowMap, vec3(shadowCoord.st + off, layer) ).r; // get depth map distance to light at coord st + off
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) // if opaque & current depth > than closest obstacle
        {
            shadow = shadow_ambient;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc, float layer) {
    ivec2 texDim = textureSize(shadowMap, 0).xy; // get depth map dimension
    float scale = 1.0;
    float dx = scale * 1.0 / float(texDim.x); // x offset = (1 / width) * scale
    float dy = scale * 1.0 / float(texDim.y); // y offset = (1 / height) * scale

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++) // -1, 0, 1
    {
        for (int y = -range; y <= range; y++) // -1, 0, 1
        {
            // float index = pseudo_random(vec4(gl_FragCoord.xy, x, y));
            shadowFactor += textureProj(sc, vec2(dx * x, dy * y), layer); // coord + offset = samples center + 8 neighbours
            count++;
        }

    }
    return shadowFactor / count;
}

vec3 shadow(vec3 color, int type) {
    float layer_offset = 0;
    vec4 shadowCoord;

    if (type == 1) {
        layer_offset = depthData.num_lights[0];
    }

    for (int i=0; i < depthData.num_lights[type]; i++) {
        if (type == 0) {
            shadowCoord = biasMat * depthData.spot_mvp[i] * vec4(inFragPos, 1.0);
        } else if (type == 1) {
            shadowCoord = biasMat * depthData.directional_mvp[i] * vec4(inFragPos, 1.0);
        }

        float shadowFactor = (enablePCF == 1) ? filterPCF(shadowCoord, layer_offset + i) : textureProj(shadowCoord, vec2(0.0), layer_offset + i);
        color *= shadowFactor;
    }
    return color;
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

vec3 F_Schlick(float cosTheta) {
    vec3 F0 = mix(vec3(0.02), material.albedo.rgb, material.metallic);
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(float cosTheta, float roughness) {
    vec3 F0 = mix(vec3(0.02), material.albedo.rgb, material.metallic);
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
        vec3 L = normalize(- lightingData.dir_direction[i].xyz);
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
    vec3 A = texture(irradianceMap, N).rgb;  //  texture(irradianceMap, uv).rgb // if sampler2D

    vec3 Lo = vec3(0.0);
    Lo = spot_light(Lo, N, V);
    Lo = directional_light(Lo, N, V);

//    for (int i = 0; i < lightingData.num_lights[0]; i++) {
//        vec3 L = normalize(lightingData.spot_position[i].xyz - inFragPos);
//        vec3 C = lightingData.spot_color[i].rgb;
//
//        Lo += BRDF(L, V, N, C);
//    };

    vec2 brdf = texture(brdfMap, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 reflection = prefiltered_reflection(R, roughness).rgb;
    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 diffuse = irradiance *  material.albedo.rgb;

    vec3 F = F_SchlickR(max(dot(N, V), 0.0), roughness);

    vec3 specular = reflection * (F * brdf.x + brdf.y);

    vec3 kD = (vec3(1.0) - F) * (1.0 - material.metallic);
    vec3 ambient = (kD * diffuse + specular);

    vec3 color = ambient + Lo;
    color = color / (color + vec3(1.0)); // Reinhard operator
    color = shadow(color, 0);
    color = shadow(color, 1);

    // Convert from linear to sRGB ! Do not use for Vulkan !
    // color = pow(color, vec3(0.4545)); // Gamma correction

    outFragColor = vec4(color, 1.0);
}
