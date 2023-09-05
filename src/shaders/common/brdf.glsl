#define PI 3.1415926535897932384626433832795

/** Power 2 */
float sqr(float x) { return x * x; }

/**
* Hammersley sampling
* Low-discrepancy sampling method based on the Hammersley sequence.
* Based on the construction of Van der Corput radical inverse from "Hacker's Delight" [Warren, Henry S.]
*/
vec2 hammersley(uint i, uint N) {
    //  Van der Corput radical inverse Φ_2(i)
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    // Compute x_i
    return vec2(float(i)/float(N), rdi);
}

/**
* Normal distribution function (NDF)
* aka. Trowbridge-Reitz GGX (ground glass unknown) distribution
* alpha = roughness * roughness;
*/
float D_GGX(float dotNH, float alpha) {
    float alpha_sqr = sqr(alpha);
    float cos_sqr = sqr(dotNH);
    float denom = cos_sqr * (alpha_sqr - 1.0) + 1.0;
    return alpha_sqr / (PI * sqr(denom));
}

/**
* Geometric attenuation function - Used for analytic light sources
* aka. Schlick-Smith GGX
* Analytic light sources : k = sqr(roughness + 1.0) / 8.0;
* mapping rougness -> roughness + 1 / 2
* k = alpha / 2
*/
float G_GGX(float dotNL, float dotNV, float roughness) {
    float k = sqr(roughness + 1.0) / 8.0;
    float ggx_l = dotNL / (dotNL * (1.0 - k) + k);
    float ggx_v = dotNV / (dotNV * (1.0 - k) + k);
    return ggx_l * ggx_v;
}

/**
* Geometric attenuation function - Used for Image Based Lighting
* aka. Schlick-Smith GGX
* IBL : k = sqr(roughness) / 2.0
* k = alpha / 2
*/
float G_IBL_GGX(float dotNL, float dotNV, float roughness) {
    float k = sqr(roughness) / 2.0;
    float ggx_l = dotNL / (dotNL * (1.0 - k) + k);
    float ggx_v = dotNV / (dotNV * (1.0 - k) + k);
    return ggx_l * ggx_v;
}

/**
* Fresnel function
* Schlick's approximation + Spherical Gaussian approximation
* vec3 F0 = mix(vec3(0.04), albedo, metallic)
*/
vec3 F_GGX(vec3 F0, float cosTheta) {
    float coeff = (-5.55473 * cosTheta - 6.98316) * cosTheta;
    return F0 + (1.0 - F0) * pow(2.0, coeff);
}

/**
* Fresnel function
* Schlick's approximation
* vec3 F0 = mix(vec3(0.04), albedo, metallic)
*/
vec3 F_Schlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 F_SchlickR(vec3 F0, float cosTheta, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

/**
* Importance sampling
* Used to solve radiance integral (IBL)
*/
vec3 importance_sample_GGX(vec3 N, vec2 Xi, float roughness) {
    float alpha = roughness * roughness;

    float phi = 2 * PI * Xi.x;
    float cosTheta = sqrt( (1.0 - Xi.y) / (1.0 + (sqr(alpha) - 1.0) * Xi.y) );
    float sinTheta = sqrt(1 - sqr(cosTheta)); // 1 = sin^2 + cos^2

    // from spherical coordinates to cartesian coordinates
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // from tangent-space vector to world-space sample vector
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0); // vec3(0.0, 1.0, 0.0) ?
    vec3 tangentX = normalize(cross(up, N));
    vec3 tangentY = cross(N, tangentX);

    return tangentX * H.x + tangentY * H.y + N * H.z;
}