#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outFragPos;
layout (location = 4) out vec3 outCameraPos;
layout (location = 5) out vec3 outViewPos;
layout (location = 6) out vec4 outTangent;

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

void main()
{
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * nodeData.model;
    mat4 transformMatrix = (cameraData.proj * cameraData.view * modelMatrix);
    vec4 pos = modelMatrix * vec4(vPosition.xyz, 1.0f);
    float f = cameraData.flip ? -1.0f : 1.0f;

    gl_Position = transformMatrix * vec4(vPosition , 1.0f);
    // gl_Position.y = f * gl_Position.y; 

    outColor = vColor;
    outUV = vUV;
    outNormal = normalize(modelMatrix * vec4(vNormal, 0.0f)).xyz; // instead of outNormal = vNormal;
    outTangent = normalize(modelMatrix * vTangent); // instead of outTangent = vTangent;
    outFragPos = pos.xyz; // vec3(modelMatrix * vec4(vPosition.xyz , 1.0f));
    outCameraPos = cameraData.pos;
    outViewPos = (cameraData.view * vec4(pos.xyz, 1.0)).xyz;
}
