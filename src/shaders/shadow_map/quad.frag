#version 450
#extension GL_EXT_debug_printf : enable
// #extension GL_ARB_shading_language_include : require

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 1) uniform sampler2D samplerDepth;

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
    float x = texture(samplerDepth, inUV).r;
    // debugPrintfEXT("Depth : %f", x);
    outFragColor = vec4(vec3(x), 1.0);
}