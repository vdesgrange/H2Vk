#version 450
#extension GL_EXT_debug_printf : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

layout (std140, set = 0, binding = 0) uniform UBO {
    mat4 depthMVP;
} ubo;

void main() {
    gl_Position =  ubo.depthMVP * vec4(vPosition, 1.0);
}