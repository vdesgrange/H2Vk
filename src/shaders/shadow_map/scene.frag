#version 460

#define ambient 0.1

const int MAX_LIGHT = 8;
const int enablePCF = 1;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;

layout (location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 1) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_position[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

layout (std140, set = 0, binding = 2) uniform ShadowData {
    layout(offset = 0) vec2  num_lights;
    layout(offset = 16) mat4 directional_mvp[MAX_LIGHT];
    layout(offset = 528) mat4 spot_mvp[MAX_LIGHT];
} depthData;

layout (set = 0, binding = 6) uniform sampler2DArray shadowMap;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );

float pseudo_random(vec4 co) {
    float dot_product = dot(co, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float textureProj(vec4 P, vec2 off, float layer) {
    vec4 shadowCoord = P / P.w;
    float cosTheta = clamp(dot(inNormal, -inFragPos), 0.0, 1.0);
    float bias = 0.005 * tan(acos(cosTheta));

    bias = clamp(bias, 0, 0.01);

    float shadow = 1.0; // default coefficient, bias handled outside.
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) // depth in valid [-1, 1] interval
    {
        float dist = texture(shadowMap, vec3(shadowCoord.st + off, layer) ).r; // get depth map distance to light at coord st + off
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) // if opaque & current depth > than closest obstacle
        {
            shadow = ambient;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc, float layer) {
    ivec2 texDim = textureSize(shadowMap, 0).xy; // get depth map dimension
    float scale = 1.0;
    float dx = scale * 1.0 / float(texDim.x); // x offset = (1 / width) * scale
    float dy = scale * 1.0 / float(texDim.y); // y offset = (1 / height) * scale

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++) // -1, 0, 1
    {
        for (int y = -range; y <= range; y++) // -1, 0, 1
        {
            // float index = pseudo_random(vec4(gl_FragCoord.xy, x, y));
            shadowFactor += textureProj(sc, vec2(dx * x, dy * y), layer); // coord + offset = samples center + 8 neighbours
            count++;
        }

    }
    return shadowFactor / count;
}

vec3 shadow(vec3 color, int type) {
    mat4 mvp[MAX_LIGHT];

    if (type == 0) {
        mvp = depthData.spot_mvp;
    } else if (type == 1) {
        mvp = depthData.directional_mvp;
    }

    for (int i=0; i < depthData.num_lights[type]; i++) {
        vec4 shadowCoord = biasMat * depthData.spot_mvp[i] * vec4(inFragPos, 1.0);
        float shadowFactor = (enablePCF == 1) ? filterPCF(shadowCoord, i) : textureProj(shadowCoord, vec2(0.0), i);
        color *= shadowFactor;
    }
    return color;
}

vec3 spot_light(vec3 color, vec3 N, vec3 V) {
    float lightCosInnerAngle = cos(radians(15.0));
    float lightCosOuterAngle = cos(radians(25.0));
    float lightRange = 100.0;

    for (int i=0; i < lightingData.num_lights[0]; i++) {
        vec3 L = lightingData.spot_position[i].xyz - inFragPos;
        float dist = length(L);
        L = normalize(L);

        vec3 dir = normalize(lightingData.spot_position[i].xyz - lightingData.spot_target[i].xyz);

        float cosDir = dot(L, dir);
        float spotEffect = smoothstep(lightCosOuterAngle, lightCosInnerAngle, cosDir);
        float heightAttenuation = smoothstep(lightRange, 0.0f, dist);

        vec3 diffuse = max(dot(N, L), ambient) * inColor;

        vec3 R = normalize(-reflect(L, N));
        float NdotR = max(0.0, dot(R, V));
        vec3 spec = vec3(pow(NdotR, 16.0) * 2.5);

        color += ((diffuse + spec) * spotEffect * heightAttenuation) * lightingData.spot_color[i].rgb * inColor;
    }

    return color;
}

void main()
{
    vec3 N = normalize(inNormal);
    vec3 V = normalize(inCameraPos - inFragPos);
    vec3 color = inColor * ambient;

    color = spot_light(color, N, V);
    color = shadow(color, 0);

    outFragColor = vec4(color, 1.0);
}