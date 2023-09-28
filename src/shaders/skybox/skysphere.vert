#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec2 outUV;

layout(std140, set = 0, binding = 1) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos;
    bool flip;
} cameraData;

void main()
{
    mat4 transformMatrix = cameraData.proj * cameraData.view;
    transformMatrix[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f); //  Cancel translation
    vec4 clipPos = transformMatrix * vec4(vPosition , 1.0f);
    gl_Position = clipPos.xyww; // fix depth with VK_COMPARE_OP_LESS_OR_EQUAL

    float coeff = (cameraData.flip == true) ? -1.0 : 1.0;
    outUV = coeff * vUV;
}