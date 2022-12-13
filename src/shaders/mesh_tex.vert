#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;

layout(set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} cameraData;

struct ObjectData {
    mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData primitives[];
} objectBuffer;

void main()
{
    mat4 modelMatrix = objectBuffer.primitives[gl_BaseInstance].model;
    mat4 transformMatrix = (cameraData.viewproj * modelMatrix);
    gl_Position = transformMatrix * vec4(vPosition , 1.0f);

    outColor = vColor;
    outUV = vUV;
    outNormal = vNormal;
}
