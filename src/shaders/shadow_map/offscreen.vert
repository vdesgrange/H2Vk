#version 460
#extension GL_EXT_debug_printf : disable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

const int MAX_SHADOW_LIGHT = 8;
const int MAX_LIGHT = 8;

//layout (std140, set = 0, binding = 0) uniform UBO {
//    mat4 depthMVP;
//} ubo;

layout (std140, set = 0, binding = 0) uniform ShadowData {
    layout(offset = 0) uint  num_lights;
    layout(offset = 16) mat4 directionalMVP[MAX_SHADOW_LIGHT];
} shadowData;

struct ObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;


layout (push_constant) uniform NodeModel {
    mat4 model;
} nodeData;

void main() {
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * nodeData.model;
    // mat4 transformMatrix = ubo.depthMVP * modelMatrix;
    mat4 transformMatrix = shadowData.directionalMVP[0] * modelMatrix;
    gl_Position =  transformMatrix * vec4(vPosition, 1.0);
}