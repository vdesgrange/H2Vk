#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common/constants.glsl"
#include "../common/filters.glsl"
#include "../common/debug.glsl"

layout(std140, set = 0, binding = 2) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

layout (std140, set = 0, binding = 3) uniform ShadowData {
    layout(offset = 0) mat4 cascadeVP[CASCADE_COUNT];
    layout(offset = 256) vec4 splitDepth;
    layout(offset = 272) bool color_cascades;
} depthData;

layout(set = 2, binding = 0) uniform sampler2D samplerAlbedoMap;
layout(set = 2, binding = 1) uniform sampler2D samplerNormalMap;
layout(set = 2, binding = 2) uniform sampler2D samplerMetalRoughnessMap;
layout(set = 2, binding = 3) uniform sampler2D samplerAOMap;
layout(set = 2, binding = 4) uniform sampler2D samplerEmissiveMap;
layout (set = 0, binding = 7) uniform sampler2DArray shadowMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos; // fragment/world position
layout (location = 4) in vec3 inCameraPos; // camera/view position
layout (location = 5) in vec3 inViewPos;
layout (location = 6) in vec4 inTangent;

layout (location = 0) out vec4 outFragColor;


void main() {
    vec3 albedo = texture(samplerAlbedoMap, inUV).rgb;

    float alpha = texture(samplerAlbedoMap, inUV).a;
	if (alpha < 0.5) {
	    discard;
	}

	 // Get cascade index for the current fragment's view position
	 uint cascadeIndex = 0;
	 for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
	 	if(inViewPos.z < depthData.splitDepth[i]) {
	 		cascadeIndex = i + 1;
	 	}
	 }

	 // Directional light
	 vec3 N = normalize(inNormal);
	 vec3 L = normalize(lightingData.dir_direction[0].xyz);
	 vec3 H = normalize(L + inViewPos);
	 float diffuse = max(dot(N, L), 0.1f);
	 vec3 lightColor = vec3(1.0);

     // Depth compare for shadowing
    vec4 shadowCoord = (biasMat * depthData.cascadeVP[cascadeIndex]) * vec4(inFragPos, 1.0);
    float shadow = texture_projection(shadowMap, N, L, shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);

    vec3 color = max(lightColor * (diffuse * albedo.rgb), vec3(0.0));
	color.rgb *= shadow;

	 // Color cascades (if enabled)
    debug_cascades(depthData.color_cascades, cascadeIndex, color);

    outFragColor = vec4(color, alpha);
}