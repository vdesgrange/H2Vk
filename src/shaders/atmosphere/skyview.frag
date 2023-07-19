#version 460
#extension GL_EXT_debug_printf : disable
#extension GL_GOOGLE_include_directive : enable

#include "./functions.glsl"

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos;
    bool flip;
} cameraData;

layout (push_constant) uniform NodeModel {
    mat4 model;
} nodeData;

void main() {
}
