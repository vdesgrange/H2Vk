#version 460
#extension GL_EXT_debug_printf : disable

layout(set = 0, binding = 0) uniform sampler2D atmosphereLUT;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = vec4(texture(atmosphereLUT, uv).rgb, 1.0);
}