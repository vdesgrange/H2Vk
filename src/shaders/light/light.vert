#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 3) in vec3 vColor;

layout (location = 0) out vec3 outColor;

layout(set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos;
} cameraData;

layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;

void main()
{
    vec3 lightPos = sceneData.sunlightDirection.xyz;
    mat4 scale = mat4(1.f); // scale factor
    mat4 translation = mat4(1.0);
    translation[3] = vec4(lightPos, 1.0f);
    mat4 modelMatrix = scale * translation;

    mat4 transformMatrix = (cameraData.proj * cameraData.view * modelMatrix);
    gl_Position = transformMatrix * vec4(vPosition.xyz, 1.0f);

    outColor = vColor;
}
