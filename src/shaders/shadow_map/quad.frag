#version 450
#extension GL_EXT_debug_printf : disable

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2DArray samplerDepth;

layout (location = 0) out vec4 outFragColor;

float linearize_depth(float depth)
{
    float n = 0.1f;
    float f = 100.0f;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

void main()
{
    float x = texture(samplerDepth, vec3(inUV, 0)).r;
    outFragColor = vec4(vec3(linearize_depth(x)), 1.0); // orthographic
}