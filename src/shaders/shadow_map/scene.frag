#version 450

#define ambient 0.1

layout (set = 0, binding = 6) uniform sampler2DArray shadowMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in vec4 inShadowCoord;

layout (location = 0) out vec4 outFragColor;

float pseudo_random(vec4 co) {
    float dot_product = dot(co, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float textureProj(vec4 shadowCoord, vec2 off) {
    float cosTheta = clamp(dot(inNormal, inViewVec), 0.0, 1.0);
    float bias = 0.005 * tan(acos(cosTheta));
    bias = clamp(bias, 0, 0.01);

    float shadow = 1.0; // default coefficient, bias handled outside.
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) // depth in valid [-1, 1] interval
    {
        float dist = texture( shadowMap, vec3(shadowCoord.st + off, 0) ).r; // get depth map distance to light at coord st + off
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) // if opaque & current depth > than closest obstacle
        {
            shadow = ambient;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc) {
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
            shadowFactor += textureProj(sc, vec2(dx * x, dy * y)); // coord + offset = samples center + 8 neighbours
            count++;
        }

    }
    return shadowFactor / count;
}

void main()
{
    int enablePCF = 1;
    float shadow = (enablePCF == 1) ? filterPCF(inShadowCoord / inShadowCoord.w) : textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));
    vec3 N = normalize(inNormal);
    vec3 L = normalize(inLightVec);
    vec3 V = normalize(inViewVec);
    vec3 R = normalize(-reflect(L, N));
    vec3 diffuse = max(dot(N, L), ambient) * inColor;

    outFragColor = vec4(diffuse * shadow, 1.0);
}