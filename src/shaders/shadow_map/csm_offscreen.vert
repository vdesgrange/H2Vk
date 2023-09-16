#version 460
#extension GL_EXT_debug_printf : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

const int CASCADE_COUNT = 4; // layout (constant_id = 0)

layout (std140, set = 0, binding = 0) uniform ShadowData {
    mat4 cascadeMV[CASCADE_COUNT];
    vec4 splitDepth;
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
    mat4 transformMatrix = shadowData.cascadeMV[pushData.cascadeIndex] * modelMatrix;

    // debugPrintfEXT("Test %i", pushData.cascadeIndex);

    outUV = vUV;
	gl_Position =  shadowData.cascadeMV[pushData.cascadeIndex] * vec4(vec3(modelMatrix * vec4(vPosition, 1.0)), 1.0);
    // gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}