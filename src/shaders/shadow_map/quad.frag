#version 450
#extension GL_EXT_debug_printf : enable

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D samplerDepth;

layout (location = 0) out vec4 outFragColor;

float linearize_depth(float depth)
{
    float n = 0.1f;
    float f = 20.0f;
    float z = 2.0 * depth - 1.0;
    return (2.0 * n * f) / (f + n - z * (f - n));
}

void main()
{
    float x = texture(samplerDepth, inUV).r;
    // outFragColor = vec4(vec3(1.0 - linearize_depth(x) / 100.0f), 1.0);
    outFragColor = vec4(vec3(x), 1.0); // orthographic
}