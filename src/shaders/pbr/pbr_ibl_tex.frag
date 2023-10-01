#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../common/constants.glsl"
#include "../common/brdf.glsl"
#include "../common/filters.glsl"
#include "../common/tonemaps.glsl"
#include "../common/debug.glsl"

layout(std140, set = 0, binding = 2) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

//layout (std140, set = 0, binding = 2) uniform ShadowData {
//    layout(offset = 0) vec2  num_lights;
//    layout(offset = 16) mat4 directional_mvp[MAX_LIGHT];
//    layout(offset = 528) mat4 spot_mvp[MAX_LIGHT];
//} depthData;

layout (std140, set = 0, binding = 0) uniform EnabledFeaturesData {
    bool shadowMapping;
    bool skybox;
    bool atmosphere;
} enabledFeaturesData;

//layout (std140, set = 0, binding = 3) uniform ShadowData {
//    mat4 cascadeVP[CASCADE_COUNT];
//    vec4 splitDepth;
//    bool color_cascades;
//} depthData;


layout(set = 0, binding = 4) uniform samplerCube irradianceMap; // aka. environment map
layout(set = 0, binding = 5) uniform samplerCube prefilteredMap;
layout(set = 0, binding = 6) uniform sampler2D brdfMap;
//layout(set = 0, binding = 7) uniform sampler2DArray shadowMap;

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
layout (location = 5) in vec3 inViewPos;
layout (location = 6) in vec4 inTangent;

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
//        float shadowFactor = (enablePCF == 1) ? filterPCF(shadowMap, inNormal, inFragPos, shadowCoord, layer_offset + i) : texture_projection(shadowMap, inNormal, inFragPos, shadowCoord, vec2(0.0), layer_offset + i);
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

vec3 BRDF(vec3 N, vec3 L, vec3 V, vec3 C, vec3 albedo, float roughness, float metallic) {
    vec3 color = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);
    float dotHV = clamp(dot(H, V), 0.0, 1.0);

    if (dotNL > 0.0) {
        float D = D_GGX(dotNH, roughness * roughness);
        float G = G_GGX(dotNL, dotNV, roughness);
        vec3 F = F_Schlick(F0, dotHV);
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

vec3 spot_light(vec3 Lo, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic) {
    for (int i = 0; i < lightingData.num_lights[0]; i++) {
        vec3 L = normalize(lightingData.spot_position[i].xyz - inFragPos);
        vec3 C = lightingData.spot_color[i].rgb;

        Lo += BRDF(L, V, N, C, albedo, roughness, metallic);
    };

    return Lo;
}

vec3 directional_light(vec3 Lo, vec3 N, vec3 V, vec3 albedo, float roughness, float metallic) {
    for (int i = 0; i < lightingData.num_lights[1]; i++) {
        vec3 L = normalize(- lightingData.dir_direction[i].xyz);
        vec3 C = lightingData.dir_color[i].rgb / 255.0;

        Lo += BRDF(L, V, N, C, albedo, roughness, metallic);
    };

    return Lo;
}

//float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex)
//{
//	float shadow = 1.0;
//	float bias = 0.005;
//
//	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
//		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
//		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
//			shadow = 0.1f;
//		}
//	}
//	return shadow;
//
//}

void main()
{

    // vec4 color = texture(samplerAlbedoMap, inUV);
	// if (color.a < 0.5) {
	// 	discard;
	// }

	// // Get cascade index for the current fragment's view position
	// uint cascadeIndex = 0;
	// for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
	// 	if(inViewPos.z < depthData.splitDepth[i]) {	
	// 		cascadeIndex = i + 1;
	// 	}
	// }

	// // Depth compare for shadowing
	// vec4 shadowCoord = (biasMat * depthData.cascadeVP[cascadeIndex]) * vec4(inFragPos, 1.0);	

	// float shadow = 0;
	// shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);

	// // Directional light
	// vec3 N = normalize(inNormal);
	// vec3 L = normalize(lightingData.dir_direction[0].xyz);
	// vec3 H = normalize(L + inViewPos);
	// float diffuse = max(dot(N, L), 0.1f);
	// vec3 lightColor = vec3(1.0);
	// outFragColor.rgb = max(lightColor * (diffuse * color.rgb), vec3(0.0));
	// outFragColor.rgb *= shadow;
	// outFragColor.a = color.a;

	// // Color cascades (if enabled)
	// if (depthData.color_cascades) {
	// 	switch(cascadeIndex) {
	// 		case 0 : 
	// 			outFragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
	// 			break;
	// 		case 1 : 
	// 			outFragColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
	// 			break;
	// 		case 2 : 
	// 			outFragColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
	// 			break;
	// 		case 3 : 
	// 			outFragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
	// 			break;
	// 	}
	// }

    float alpha = texture(samplerAlbedoMap, inUV).a;
    if (alpha < 0.5) {
        discard;
    }

    vec3 albedo = pow(texture(samplerAlbedoMap, inUV).rgb, vec3(2.2)); // gamma correction
    vec3 normal = texture(samplerNormalMap, inUV).rgb;
    float roughness = texture(samplerMetalRoughnessMap, inUV).g;
    float metallic = texture(samplerMetalRoughnessMap, inUV).b;
    float ao = texture(samplerAOMap, inUV).r;
    vec3 emissive = texture(samplerEmissiveMap, inUV).rgb;

    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 N = calculateNormal();
    vec3 R = reflect(-V, N);

    vec2 uv = sample_spherical_map(N);

    vec3 Lo = vec3(0.0);
    // Lo = spot_light(Lo, N, V, albedo, roughness, metallic);
    Lo = directional_light(Lo, N, V, albedo, roughness, metallic);

    vec2 brdf = texture(brdfMap, vec2(max(dot(N, V), 0.01), roughness)).rg;
    vec3 reflection = prefiltered_reflection(R, roughness).rgb;
    vec3 irradiance = texture(irradianceMap, N).rgb;

    vec3 diffuse = irradiance * albedo;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F_SchlickR(F0, max(dot(N, V), 0.0), roughness);

    vec3 specular = reflection * (F * brdf.x + brdf.y);

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 ambient = (kD * diffuse + specular + emissive) * texture(samplerAOMap, inUV).rrr;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0)); // Reinhard operator
    color = color * (1.0f / uncharted2_tonemap(vec3(11.2f)));

//    if (enabledFeaturesData.shadowMapping) {
//        // Cascaded shadow mapping
//        uint cascadeIndex = 0;
//        for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
//            if(inViewPos.z < depthData.splitDepth[i]) {
//                cascadeIndex = i + 1;
//            }
//        }
//
//        vec4 coord = biasMat * depthData.cascadeVP[cascadeIndex] * vec4(inFragPos, 1.0);
//        vec3 L = normalize(lightingData.dir_direction[0].xyz);
//        float shadow = texture_projection(shadowMap, N, L, coord / coord.w, vec2(0.0), cascadeIndex);
//        color.rgb *= shadow;
//
//        debug_cascades(depthData.color_cascades, cascadeIndex, color);
//    }

    outFragColor = vec4(color, 1.0);
}
