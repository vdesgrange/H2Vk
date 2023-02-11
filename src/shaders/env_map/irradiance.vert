#version 460

layout(push_constant) uniform View {
    mat4 viewProj;
} view;

layout (location = 0) in vec3 vPosition;

layout (location = 0) out vec3 outPos;

void main()
{
    outPos = vPosition;
    gl_Position = viewProj * vec4(vPosition , 1.0f);
}