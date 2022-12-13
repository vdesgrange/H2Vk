#version 460

layout(set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D samplerColorMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);;
    outFragColor = vec4(color.rgb, 1.0f);
}
