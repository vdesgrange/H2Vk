#version 460

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor;
    vec4 fogDistances;
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;


void main()
{
    float lightFactor = sceneData.sunlightDirection.w;
    vec3 lightColor = sceneData.sunlightColor.xyz;
    vec3 result = lightFactor * lightColor * inColor;
    outFragColor = vec4(result.rgb, 1.0);
}
