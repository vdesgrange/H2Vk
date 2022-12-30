#version 460

layout (location = 0) out vec3 outPos; // fragment position

layout(set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;

layout(set = 0, binding = 1) uniform SceneData {
    vec4 fogColor;
    vec4 fogDistances;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} sceneData;


void main()
{
    vec3 vPosition = vec3(sceneData.sunlightDirection);
    gl_Position = cameraData.viewproj * vec4(vPosition , 1.0f);

    outPos = vPosition.xyz;
}
