#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

layout (std140, set = 0, binding = 0) uniform ShadowData {
    layout(offset = 0) mat4 cascadeVP[CASCADE_COUNT];
    layout(offset = 256) vec4 splitDepth;
} shadowData;

struct ObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;

layout (push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) int cascadeIndex;
} pushData;

void main()
{
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * pushData.model;
    mat4 transformMatrix = shadowData.cascadeVP[pushData.cascadeIndex] * modelMatrix;

    outUV = vUV;
    gl_Position = transformMatrix * vec4(vPosition, 1.0);
}