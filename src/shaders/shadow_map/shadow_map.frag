#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../common/constants.glsl"

#define ambient 0.1

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;
layout (location = 5) in vec3 inViewPos;

layout (location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 2) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

//layout (std140, set = 0, binding = 3) uniform ShadowData {
//    mat4 cascadeVP[CASCADE_COUNT];
//    vec4 splitDepth;
//    bool color_cascades;
//} depthData;

//layout (set = 0, binding = 7) uniform sampler2DArray shadowMap;

//float texture_projection(vec4 coord, vec2 offset, float layer) {
//    float shadow = 1.0;
//    float bias = 0.005;
//
//    // float cosTheta = clamp(dot(normalize(inNormal), normalize(lightingData.dir_direction[0].xyz)), 0.0, 1.0);
//    // float bias = 0.005 * tan(acos(cosTheta));
//    // bias = clamp(bias, 0, 0.1);
//
//    if ( coord.z >= -1.0 && coord.z <= 1.0 )
//    {
//        float dist = texture(shadowMap, vec3(coord.st + offset, layer) ).r;
//        if ( coord.w > 0.0 && dist < coord.z - bias)
//        {
//            shadow = ambient;
//        }
//    }
//    return shadow;
//}

void main()
{
    uint cascadeIndex = 0;
//    for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
//        if(inViewPos.z < depthData.splitDepth[i]) {
//            cascadeIndex = i + 1;
//        }
//    }
//
//    vec4 coord = biasMat * depthData.cascadeVP[cascadeIndex] * vec4(inFragPos, 1.0);
//    float shadow = texture_projection(coord / coord.w, vec2(0.0), cascadeIndex);

    vec3 N = normalize(inNormal);
    vec3 L = normalize(-lightingData.dir_direction[0].xyz);

    float diffuse = max(dot(N, L), ambient);
    vec3 lightColor = vec3(1.0);
    outFragColor.rgb = max(lightColor * (diffuse * vec3(1.0)), vec3(0.0));
//    outFragColor.rgb *= shadow;
    outFragColor.a = 1.0;

//    if (depthData.color_cascades) {
        switch(cascadeIndex) {
            case 0 :
            outFragColor.rgb *= vec3(1.0f, 0.25f, 0.25f); // red
            break;
            case 1 :
            outFragColor.rgb *= vec3(0.25f, 1.0f, 0.25f); // green
            break;
            case 2 :
            outFragColor.rgb *= vec3(0.25f, 0.25f, 1.0f); // blue
            break;
            case 3 :
            outFragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
            break;
        }
//    }

}