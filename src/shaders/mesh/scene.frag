#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos; // fragment position
layout (location = 4) in vec3 inCameraPos; // camera/view position

layout (location = 0) out vec4 outFragColor;

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
    float lightFactor = sceneData.sunlightDirection.w;
    vec3 lightColor = sceneData.sunlightColor.xyz;

    vec3 N = normalize(inNormal);
    vec3 L = normalize(lightPos - inFragPos);
    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 R = reflect(-L, N);  // Phong
    vec3 H = normalize(L + V); // Halfway

    vec3 ambient = lightFactor * lightColor;
    vec3 diffuse = max(dot(N, L), 0.0) * inColor;
    // vec3 specular = vec3(sceneData.specularFactor) * pow(max(dot(R, V), 0.0), 32.0);  // Phong
    vec3 specular = vec3(sceneData.specularFactor) * pow(max(dot(N, H), 0.0), 32.0); // Blinn-Phong

    vec3 result = (ambient + diffuse) * inColor;
    outFragColor = vec4(result.rgb, 1.0); // color.rgb

    // outFragColor = vec4(inColor, 1.0f); // inColor + sceneData.ambientColor.xyz
}