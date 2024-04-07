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
layout (location = 6) in vec3 inTangent;

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
        vec3 L = normalize(lightingData.dir_direction[i].xyz  - inFragPos);
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
    vec3 R = -normalize(reflect(V, N));

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

// #version 460
// #extension GL_GOOGLE_include_directive : enable

// #include "../common/constants.glsl"

// layout (location = 0) in vec3 inWorldPos;
// layout (location = 1) in vec3 inNormal;
// layout (location = 2) in vec2 inUV;

// layout(std140, set = 0, binding = 1) uniform  CameraBuffer
// {
//     mat4 view;
//     mat4 projection;
//     vec3 camPos;
//     bool flip;
// } cameraData;


// layout(std140, set = 0, binding = 2) uniform LightingData {
//     layout(offset = 0) vec2 num_lights;
//     layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
//     layout(offset = 144) vec4 dir_color[MAX_LIGHT];
//     layout(offset = 272) vec4 spot_position[MAX_LIGHT];
//     layout(offset = 400) vec4 spot_target[MAX_LIGHT];
//     layout(offset = 528) vec4 spot_color[MAX_LIGHT];
// } lightingData;

// layout (push_constant) uniform Material {
//     layout(offset = 64) vec4 albedo; // update offset for csm
//     layout(offset = 80) float metallic;
//     layout(offset = 84) float roughness;
//     layout(offset = 88) float ao;
// } material;

// layout(set = 0, binding = 4) uniform samplerCube samplerIrradiance; // aka. environment map
// layout(set = 0, binding = 5) uniform samplerCube prefilteredMap; // aka. prefiltered map
// layout(set = 0, binding = 6) uniform sampler2D samplerBRDFLUT; // aka. prefilter map
// layout(set = 0, binding = 7) uniform sampler2DArray shadowMap;

// layout (location = 0) out vec4 outColor;

// #define PI 3.1415926535897932384626433832795

// // From http://filmicgames.com/archives/75
// vec3 Uncharted2Tonemap(vec3 x)
// {
// 	float A = 0.15;
// 	float B = 0.50;
// 	float C = 0.10;
// 	float D = 0.20;
// 	float E = 0.02;
// 	float F = 0.30;
// 	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
// }

// // Normal Distribution function --------------------------------------
// float D_GGX(float dotNH, float roughness)
// {
// 	float alpha = roughness * roughness;
// 	float alpha2 = alpha * alpha;
// 	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
// 	return (alpha2)/(PI * denom*denom); 
// }

// // Geometric Shadowing function --------------------------------------
// float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
// {
// 	float r = (roughness + 1.0);
// 	float k = (r*r) / 8.0;
// 	float GL = dotNL / (dotNL * (1.0 - k) + k);
// 	float GV = dotNV / (dotNV * (1.0 - k) + k);
// 	return GL * GV;
// }

// // Fresnel function ----------------------------------------------------
// vec3 F_Schlick(float cosTheta, vec3 F0)
// {
// 	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
// }
// vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
// {
// 	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
// }

// vec3 prefilteredReflection(vec3 R, float roughness)
// {
// 	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
// 	float lod = roughness * MAX_REFLECTION_LOD;
// 	float lodf = floor(lod);
// 	float lodc = ceil(lod);
// 	vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
// 	vec3 b = textureLod(prefilteredMap, R, lodc).rgb;
// 	return mix(a, b, lod - lodf);
// }

// vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness)
// {
// 	// Precalculate vectors and dot products	
// 	vec3 H = normalize (V + L);
// 	float dotNH = clamp(dot(N, H), 0.0, 1.0);
// 	float dotNV = clamp(dot(N, V), 0.0, 1.0);
// 	float dotNL = clamp(dot(N, L), 0.0, 1.0);

// 	// Light color fixed
// 	vec3 lightColor = vec3(1.0);

// 	vec3 color = vec3(0.0);

// 	if (dotNL > 0.0) {
// 		// D = Normal distribution (Distribution of the microfacets)
// 		float D = D_GGX(dotNH, roughness); 
// 		// G = Geometric shadowing term (Microfacets shadowing)
// 		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
// 		// F = Fresnel factor (Reflectance depending on angle of incidence)
// 		vec3 F = F_Schlick(dotNV, F0);		
// 		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);		
// 		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);			
// 		color += (kD * material.albedo.rgb / PI + spec) * dotNL;
// 	}

// 	return color;
// }

// void main()
// {		
// 	vec3 N = normalize(inNormal);
// 	vec3 V = normalize(cameraData.camPos - inWorldPos);
// 	vec3 R = reflect(-V, N); 

// 	float metallic = material.metallic;
// 	float roughness = material.roughness;

// 	vec3 F0 = vec3(0.04); 
// 	F0 = mix(F0, material.albedo.rgb, metallic);

// 	vec3 Lo = vec3(0.0);
//     for (int i = 0; i < lightingData.num_lights[1]; i++) {
//         vec3 L = normalize(lightingData.dir_direction[i].xyz - inWorldPos);
// 		Lo += specularContribution(L, V, N, F0, metallic, roughness);
// 	}   
	
// 	vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
// 	vec3 reflection = prefilteredReflection(R, roughness).rgb;	
// 	vec3 irradiance = texture(samplerIrradiance, N).rgb;

// 	// Diffuse based on irradiance
// 	vec3 diffuse = irradiance * material.albedo.rgb;	

// 	vec3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

// 	// Specular reflectance
// 	vec3 specular = reflection * (F * brdf.x + brdf.y);

// 	// Ambient part
// 	vec3 kD = 1.0 - F;
// 	kD *= 1.0 - metallic;	  
// 	vec3 ambient = (kD * diffuse + specular);
	
// 	vec3 color = ambient + Lo;

// 	// Tone mapping
// 	color = Uncharted2Tonemap(color);
// 	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
// 	// Gamma correction
// 	color = pow(color, vec3(0.4545));

// 	outColor = vec4(color, 1.0);
// }