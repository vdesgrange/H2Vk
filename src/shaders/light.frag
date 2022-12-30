#version 460

layout(set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 ambientColor;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
} sceneData;

layout (location = 0) in vec3 inPos; // fragment position

layout (location = 0) out vec4 outFragColor;

void main()
{

    vec3 lightPos = sceneData.sunlightDirection.xyz;
    vec3 coord = inPos.xyz;

    float d = distance(lightPos, coord) / 0.5;
    float a = d > 0.99 ? 1.0 - smoothstep(0.99, 1.0, d) : 1.0;
    outFragColor = vec4(1.0, 1.0, 1.0, a);
}
