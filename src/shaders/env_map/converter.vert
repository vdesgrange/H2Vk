#version 460

layout(push_constant) uniform View {
    mat4 viewProj;
} view;

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec3 outPos;

void main()
{
    outPos = vPosition;
    gl_Position = view.viewProj * vec4(vPosition , 1.0f);
}