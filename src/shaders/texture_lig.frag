#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D tex1;

void main()
{
    vec3 color = texture(tex1, inUV).xyz;
    outFragColor = vec4(color, 1.0f);
    // outFragColor = vec4(texCoord.x, texCoord.y, 0.5f, 1.0f);
}
