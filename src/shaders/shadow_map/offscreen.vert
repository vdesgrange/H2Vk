#version 460
#extension GL_EXT_debug_printf : enable
// #extension GL_ARB_shader_viewport_layer_array : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

layout (constant_id = 0) const int MAX_LIGHT = 8;

layout (std140, set = 0, binding = 0) uniform ShadowData {
    layout(offset = 0) uint  num_lights;
    layout(offset = 16) mat4 directionalMVP[MAX_LIGHT];
} shadowData;

struct ObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;

layout (push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) int lightIndex;
    layout(offset = 68) int lightType;
} pushData;

void main() {
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * pushData.model;
    // debugPrintfEXT("light details : %i %i", pushData.lightIndex, pushData.lightType);
    mat4 transformMatrix = shadowData.directionalMVP[pushData.lightIndex] * modelMatrix;
    gl_Position =  transformMatrix * vec4(vPosition, 1.0);
}