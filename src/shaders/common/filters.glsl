#define shadow_ambient 0.2

float pseudo_random(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

/**
* One-liner random noise function (unknown origin)
* Samples the pseudo random numbers in regular intervals, result in an undersampling
* which produce together with the fraction of sin() random numbers.
*/
float pseudo_random(vec4 co){
    return fract(sin(dot(co, vec4(12.9898,78.233,45.164,94.673))) * 43758.5453);
}


/**
* Sample shadow map
* shadow coordinates = coord / coord.w;
* Divide by w to emulate perspective (w = 1 for orthographic).
*/
float texture_projection(in sampler2DArray tex, vec3 N, vec3 R, vec4 coord, vec2 off, float layer) {
    float cosTheta = clamp(dot(N, -R), 0.0f, 1.0f);
    float bias = 0.005 * tan(acos(cosTheta));
    bias = clamp(bias, 0.0f, 0.01f);

    float shadow = 1.0; // default coefficient, bias handled outside.
    if ( coord.z > 0.0 && coord.z < 1.0 ) // depth in valid [-1, 1] interval (opengl : z in [-1, 1], vulkan : z in [0, 1]?)
    {
        float dist = texture(tex, vec3(coord.st + off, layer) ).r; // get depth map distance to light at coord st + off
        if ( coord.w > 0.0 && dist < coord.z - bias) // if opaque & current depth > than closest obstacle
        {
            shadow = shadow_ambient;
        }
    }
    return shadow;
}

/**
*  Percentage-closer filtering
*/
float filterPCF(in sampler2DArray tex, vec3 N, vec3 R, vec4 coord, float layer) {
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
            shadowFactor += texture_projection(tex, N, R, coord, vec2(dx * x, dy * y), layer); // coord + offset = samples center + 8 neighbours
            count++;
        }

    }
    return shadowFactor / count;
}

/**
* Stratified poisson sampling
*/
float filterSPS(in sampler2DArray tex, vec3 N, vec3 R, vec4 coord, float layer) {
    ivec2 texDim = textureSize(tex, 0).xy; // get depth map dimension

    float shadowFactor = 0.0;
    int count = 0;

    for (int i = 0; i < 4; i++) {
        int index = int(16.0f * pseudo_random(vec4(gl_FragCoord.xyy, i))) % 16;
        shadowFactor += texture_projection(tex, N, R, coord, poissonDisk[index] / 700.0, layer);
        count++;
    }

    return shadowFactor / count;
}