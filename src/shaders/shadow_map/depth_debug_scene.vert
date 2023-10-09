#version 460
#extension GL_GOOGLE_include_directive : enable

#include "../common/constants.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec4 inTangent;

layout(set = 0, binding = 1) uniform  CameraBuffer
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
layout (location = 5) out vec3 outViewPos;
layout (location = 6) out vec4 outTangent;

void main()
{
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * nodeData.model;
    mat4 cameraMVP = (cameraData.proj * cameraData.view * modelMatrix);
    vec4 pos = modelMatrix * vec4(inPosition.xyz, 1.0f);

    outColor = inColor;
    outUV = inUV;
    outNormal = mat3(modelMatrix) * inNormal;
    outFragPos = pos.xyz;
    outCameraPos = cameraData.pos;
    outViewPos = (cameraData.view * vec4(pos.xyz, 1.0)).xyz;
    outTangent = inTangent;

    gl_Position = cameraMVP * vec4(inPosition.xyz, 1.0f);
    // gl_Position.y = -gl_Position.y;
}