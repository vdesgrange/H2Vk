#version 460

layout(std140, set = 0, binding = 1) uniform SceneData {
    vec4 fogColor; // w is for exponent
    vec4 fogDistances; //x for min, y for max, zw unused.
    vec4 sunlightDirection; //w for sun power
    vec4 sunlightColor;
    float specularFactor;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D samplerColorMap;
layout(set = 2, binding = 1) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos; // fragment position
layout (location = 4) in vec3 inCameraPos; // camera/view position
layout (location = 5) in vec4 inTangent;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 color = texture(samplerColorMap, inUV); // * vec4(inColor, 1.0); //  color null for texture

    vec3 lightPos = sceneData.sunlightDirection.xyz;
    float lightFactor = sceneData.sunlightDirection.w;
    vec3 lightColor = sceneData.sunlightColor.xyz;

    vec3 N = normalize(inNormal);
//    vec3 T = normalize(inTangent.xyz);
//    vec3 B = cross(inNormal, inTangent.xyz) * inTangent.w;
//    mat3 TBN = mat3(T, B, N);
//    N = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
    vec3 L = normalize(lightPos - inFragPos);
    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 R = reflect(-L, N);  // Phong
    vec3 H = normalize(L + V); // Halfway
    vec3 ambient = lightFactor * lightColor;
    vec3 diffuse = max(dot(N, L), 0.0)  * color.rgb; // inColor: color null for texture
    // vec3 specular = vec3(sceneData.specularFactor) * pow(max(dot(R, V), 0.0), 32.0);  // Phong
    vec3 specular = vec3(sceneData.specularFactor) * pow(max(dot(N, H), 0.0), 32.0); // Blinn-Phong

    vec3 result = (ambient + diffuse + specular) * color.rgb;
    outFragColor = vec4(result.rgb, 1.0);
}
