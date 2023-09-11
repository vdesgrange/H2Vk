#version 460

layout (location = 0) out vec2 outUV;
layout (location = 1) flat out uint outCascadeIndex;

layout (push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) uint cascadeIndex;
} pushData;

void main()
{
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    outCascadeIndex = pushData.cascadeIndex;

    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}