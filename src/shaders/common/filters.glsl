#define shadow_ambient 0.1

/**
*/
float textureProj(in sampler2DArray tex, vec3 N, vec3 R, vec4 P, vec2 off, float layer) {
    vec4 shadowCoord = P / P.w; //  w = 1 for orthographic. Divide by w to emulate perspective.
    float cosTheta = clamp(dot(N, -R), 0.0, 1.0);
    float bias = 0.005 * tan(acos(cosTheta));

    bias = clamp(bias, 0, 0.01);

    float shadow = 1.0; // default coefficient, bias handled outside.
    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) // depth in valid [-1, 1] interval
    {
        float dist = texture(tex, vec3(shadowCoord.st + off, layer) ).r; // get depth map distance to light at coord st + off
        if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - bias) // if opaque & current depth > than closest obstacle
        {
            shadow = shadow_ambient;
        }
    }
    return shadow;
}

/**
*  Percentage-closer filtering
*/
float filterPCF(in sampler2DArray tex, vec3 N, vec3 R, vec4 sc, float layer) {
    ivec2 texDim = textureSize(tex, 0).xy; // get depth map dimension
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
            shadowFactor += textureProj(tex, N, R, sc, vec2(dx * x, dy * y), layer); // coord + offset = samples center + 8 neighbours
            count++;
        }

    }
    return shadowFactor / count;
}