#version 460
#extension GL_EXT_debug_printf : disable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

layout (std140, set = 0, binding = 0) uniform UBO {
    mat4 depthMVP;
} ubo;

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
    mat4 transformMatrix = ubo.depthMVP * modelMatrix;
    gl_Position =  transformMatrix * vec4(vPosition, 1.0);
    // gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}