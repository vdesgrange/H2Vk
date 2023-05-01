#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

//layout (binding = 0) uniform UBO
//{
//    mat4 projection;
//    mat4 view;
//    mat4 model;
//    mat4 lightSpace;
//    vec4 lightPos;
//    float zNear;
//    float zFar;
//} ubo;

layout(set = 0, binding = 0) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos; // used for light post as test
    bool flip;
} cameraData;

layout(set = 0, binding = 2) uniform DepthBuffer {
    mat4 depthMVP;
} depthData;

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

layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out vec4 outShadowCoord;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );

void main()
{
    outColor = vColor;
    outUV = vUV;
    outNormal = vNormal;

    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model * nodeData.model;
    mat4 transformMatrix = (cameraData.proj * cameraData.view * modelMatrix);
    gl_Position = transformMatrix * vec4(vPosition.xyz, 1.0f);

    vec4 pos = modelMatrix * vec4(vPosition.xyz, 1.0f);
    outNormal = mat3(modelMatrix) * inNormal;

    outViewVec = -pos.xyz;
    outLightVec = normalize(cameraData.pos - vPosition);
    outShadowCoord = ( biasMat * depthData.depthMVP * modelMatrix ) * vec4(vPosition, 1.0);
}