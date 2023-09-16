#version 460

#define ambient 0.1

const int CASCADE_COUNT = 4;
const int MAX_LIGHT = 8;
const int enablePCF = 0;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inFragPos;
layout (location = 4) in vec3 inCameraPos;

layout (location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 1) uniform LightingData {
    layout(offset = 0) vec2 num_lights;
    layout(offset = 16) vec4 dir_direction[MAX_LIGHT];
    layout(offset = 144) vec4 dir_color[MAX_LIGHT];
    layout(offset = 272) vec4 spot_position[MAX_LIGHT];
    layout(offset = 400) vec4 spot_target[MAX_LIGHT];
    layout(offset = 528) vec4 spot_color[MAX_LIGHT];
} lightingData;

//layout (std140, set = 0, binding = 2) uniform ShadowData {
//    layout(offset = 0) vec2  num_lights;
//    layout(offset = 16) mat4 directional_mvp[MAX_LIGHT];
//    layout(offset = 528) mat4 spot_mvp[MAX_LIGHT];
//} depthData;

layout (std140, set = 0, binding = 2) uniform ShadowData {
    mat4 cascadeMV[CASCADE_COUNT];
    vec4 splitDepth;
    bool color_cascades;
} depthData;

layout (set = 0, binding = 6) uniform sampler2DArray shadowMap;

const mat4 biasMat = mat4(
0.5, 0.0, 0.0, 0.0,
0.0, 0.5, 0.0, 0.0,
0.0, 0.0, 1.0, 0.0,
0.5, 0.5, 0.0, 1.0 );

vec2 poissonDisk[4] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2( 0.94558609, -0.76890725 ),
    vec2( -0.094184101, -0.92938870 ),
    vec2( 0.34495938, 0.29387760 )
);

float texture_projection(vec4 coord, vec2 offset, float layer) {
    float shadow = 1.0;
    // float cosTheta = clamp(dot(normalize(inNormal), normalize(lightingData.dir_direction[0].xyz)), 0.0, 1.0);
    float bias = 0.005; // * tan(acos(cosTheta));
    // bias = clamp(bias, 0, 0.1);

    if ( coord.z > -1.0 && coord.z < 1.0 )
    {
        float dist = texture(shadowMap, vec3(coord.st + offset, layer) ).r;
        if ( coord.w > 0.0 && dist < coord.z - bias)
        {
            // shadow -= 0.2 * (1.0 - dist);
            shadow = ambient;
        }
    }
    return shadow;
}

float pseudo_random(vec4 co) {
    float dot_product = dot(co, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float filter_pcf(vec4 sc, float layer)
{
    ivec2 texDim = textureSize(shadowMap, 0).xy;
    float scale = 0.75;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 4;

    for (int i = 0; i <= range; i++)
    {
        int index = int(16.0 * pseudo_random(vec4(gl_FragCoord.xyy, i))) % 16;
        shadowFactor += texture_projection(sc, poissonDisk[index] / 700.0, layer); // coord + offset = samples center + 8 neighbours
    }
    return shadowFactor;
}

vec3 directional_light(vec3 color, vec3 N, vec3 V) {
    vec3 L = normalize(- lightingData.dir_direction[0].xyz);
    vec3 R = normalize(-reflect(L, N));
    vec3 diffuse = max(dot(N, L), ambient) * color;

    float NdotR = max(0.0, dot(R, V));
    vec3 spec = vec3(pow(NdotR, 16.0) * 2.5);

    color += (diffuse + spec) * lightingData.dir_color[0].rgb * inColor;

    return color;
}


void main()
{
    uint cascadeIndex = 0;
    for(uint i = 0; i < CASCADE_COUNT - 1; ++i) {
        if(inCameraPos.z < depthData.splitDepth[i]) {
            cascadeIndex = i + 1;
        }
    }

    vec4 coord = biasMat * depthData.cascadeMV[cascadeIndex] * vec4(inFragPos, 1.0);
    float shadow = texture_projection(coord / coord.w, vec2(0.0), cascadeIndex);

    vec3 N = normalize(inNormal);
    vec3 L = normalize(-lightingData.dir_direction[0].xyz);
    vec3 H = normalize(L + inFragPos);
    vec3 V = normalize(inCameraPos - inFragPos);
    float diffuse = max(dot(N, L), ambient);
    vec3 lightColor = vec3(1.0);
    outFragColor.rgb = max(lightColor * (diffuse * inColor.rgb), vec3(0.0));
    outFragColor.rgb *= shadow;
    outFragColor.a = 1.0;

    if (depthData.color_cascades) {
        switch(cascadeIndex) {
            case 0 :
            outFragColor.rgb *= vec3(1.0f, 0.25f, 0.25f); // red
            break;
            case 1 :
            outFragColor.rgb *= vec3(0.25f, 1.0f, 0.25f); // green
            break;
            case 2 :
            outFragColor.rgb *= vec3(0.25f, 0.25f, 1.0f); // blue
            break;
            case 3 :
            outFragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
            break;
        }
    }

    // color = directional_light(color, N, V);
//    outFragColor = vec4(color, 1.0);
}