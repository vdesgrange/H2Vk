#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outFragPos; // fragment position
layout (location = 4) out vec3 outCameraPos; // fragment position

layout(set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos;
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
    mat4 transformMatrix = (cameraData.proj * cameraData.view *  modelMatrix);
    gl_Position = transformMatrix * vec4(vPosition , 1.0f);

    outColor = vColor;
    outUV = vUV;
    outNormal = vNormal;
    outFragPos = vec3(modelMatrix * vec4(vPosition.xyz , 1.0f));
    outCameraPos = cameraData.pos;
}
