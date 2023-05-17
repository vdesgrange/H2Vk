#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout(set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos;
    bool flip;
} cameraData;

struct ObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;

layout (push_constant) uniform NodeModel {
    mat4 model;
} nodeData;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outFragPos;
layout (location = 4) out vec3 outCameraPos;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );

void main()
{
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * nodeData.model;
    mat4 cameraMVP = (cameraData.proj * cameraData.view * modelMatrix);
    vec4 pos = modelMatrix * vec4(vPosition.xyz, 1.0f);

    gl_Position = cameraMVP * vec4(vPosition.xyz, 1.0f);

    outColor = vColor;
    outUV = vUV;
    outNormal = mat3(modelMatrix) * vNormal;
    outFragPos = pos.xyz;
    outCameraPos = cameraData.pos;
}