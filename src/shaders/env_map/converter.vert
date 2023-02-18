#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_spirv_intrinsics : enable

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
    // debugPrintfEXT("ViewProj: [0] %v4f [1] %v4f [2] %v4f [3] %v4f", nodeData.viewProj[0], nodeData.viewProj[1], nodeData.viewProj[2], nodeData.viewProj[3]);
    debugPrintfEXT("Model: [0] %v4f [1] %v4f [2] %v4f [3] %v4f", nodeData.model[0], nodeData.model[1], nodeData.model[2], nodeData.model[3]);

    outPos = vPosition;
    gl_Position = nodeData.viewProj * vec4(vPosition , 1.0f);
}