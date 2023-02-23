#version 460
#extension GL_EXT_debug_printf : disable

layout (push_constant) uniform NodeModel {
    layout(offset = 0) mat4 model;
    layout(offset = 64) mat4 viewProj;
} nodeData;


layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec3 outPos;

void main()
{
    outPos = vPosition;
    gl_Position = nodeData.viewProj * vec4(vPosition , 1.0f);
}