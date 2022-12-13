#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;


layout (set = 0, binding = 0) uniform UBOScene
{
    mat4 projection;
    mat4 view;
    vec4 lightPos;
    vec4 viewPos;
} uboScene;

layout( push_constant ) uniform PushConsts {
    mat4 model;
} primitive;

void main()
{
    outNormal = vNormal;
    outColor = vColor;
    outUV = vUV;
    gl_Position = uboScene.projection * uboScene.view * primitive.model * vec4(vPosition.xyz, 1.0);

    vec4 pos = uboScene.view * vec4(vPosition, 1.0);
    outNormal = mat3(uboScene.view) * vNormal;
    vec3 lPos = mat3(uboScene.view) * uboScene.lightPos.xyz;
    outLightVec = uboScene.lightPos.xyz - pos.xyz;
    outViewVec = uboScene.viewPos.xyz - pos.xyz;
}