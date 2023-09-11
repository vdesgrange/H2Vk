#version 460

layout (location = 0) in vec2 inUV;
layout (location = 1) flat in uint inCascadeIndex;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2DArray depthMap;

float linearize_depth(float depth)
{
    float n = 0.1f;
    float f = 100.0f;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

void main()
{
    float x = texture(depthMap, vec3(inUV, float(inCascadeIndex))).r;
    outFragColor = vec4(inUV, 0.0, 1.0);
}