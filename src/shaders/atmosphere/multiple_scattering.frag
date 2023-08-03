#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "./functions.glsl"

layout(set = 0, binding = 0) uniform sampler2D transmittanceLUT;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

const int SAMPLE_DIR = 8;
const int SAMPLE_COUNT = 20;

void single_scattering(vec3 x, vec3 sun_dir, float i, float j, out vec3 Lprime, out vec3 Lf) {
    Lprime = vec3(0.0);
    Lf = vec3(0.0);
    float bias = 0.3;

    // Compute ray direction
    float theta = PI * (i + 0.5) / float(SAMPLE_DIR);
    float phi = acos(clamp(1.0 - 2.0 * (j + 0.5) / float(SAMPLE_DIR), -1.0, 1.0));
    vec3 ray_dir = get_spherical_direction(theta, phi); // ray direction omega

    float t_atmo = get_ray_intersection_length(x, ray_dir, r_top);
    float t_ground = get_ray_intersection_length(x, ray_dir, r_ground);
    float t_max = t_atmo;
    if (t_ground > 0.0) {
        t_max = t_ground;
    }

    float nu = dot(sun_dir, ray_dir); // nu, cosinus of angle between sun and ray direction
    float p_r = get_rayleigh_phase(-nu);
    float p_m = get_mie_phase(0.8, nu);
    vec3 T = vec3(1.0);
    float t = 0.0;
    float dt = t_max / SAMPLE_COUNT;

    for (int k = 0; k < SAMPLE_COUNT; k++) {
        float new_t = t_max * (k + bias) / SAMPLE_COUNT;
        dt = new_t - t;
        t = new_t;

        vec3 x_tv = x + t * ray_dir; // x - tv

        vec3 beta_s = vec3(0.0);
        vec3 beta_a = vec3(0.0);
        vec3 beta_e = vec3(0.0);
        get_beta_coefficients(x_tv, beta_s, beta_a, beta_e);

        vec3 sample_T = exp(-dt * beta_e); // T(x, x-tv), transmittance a revoir
        Lf += T * (beta_s - beta_s * sample_T) / beta_e; // integration

        vec3 beta_s_r = get_rayleigh_scattering_coefficient(x_tv);
        vec3 sigma_s_r = beta_s_r * vec3(p_r); // rayleight scattering * density * phase function
        vec3 beta_s_m = get_mie_scattering_coefficient(x_tv);
        vec3 sigma_s_m = beta_s_m * vec3(p_m); // mie scattering * density * phase function

        vec2 sun_uv = get_uv_for_LUT(x_tv, sun_dir);
        vec3 T_sun = texture(transmittanceLUT, sun_uv).xyz; // or lutTransmittanceToUV
        vec3 sigma_s = (sigma_s_r + sigma_s_m) * T_sun; // In-scattering (with phase)
        vec3 sigma_s_integral = (sigma_s - sigma_s * sample_T) / beta_e;

        Lprime += sigma_s_integral * T;
        T *= sample_T; // exp(a+b) = exp(a) * exp(b)
    }

    if (t_ground > 0.0) {
        vec3 x_hit = x + t_ground * ray_dir;
        if (dot(x, sun_dir) > 0.0) {
            x_hit = normalize(x_hit) * r_ground;
            vec2 hit_uv = get_uv_for_LUT(x_hit, sun_dir);
            Lprime += T * vec3(albedo) * texture(transmittanceLUT, hit_uv).xyz;
        }
    }
}

void main() {
    float height = uv.y * (r_top - r_ground) + r_ground;

    float zenithSunCosTheta = 2.0 * uv.x - 1.0; // mu_s cosinus zenith and sun angle
    float zenithSunSinTheta = sqrt(clamp(1.0 - zenithSunCosTheta * zenithSunCosTheta, 0.0, 1.0));

    vec3 x = vec3(0.0, height, 0.0); // world position
    vec3 sun_dir = normalize(vec3(0.0, zenithSunCosTheta, -zenithSunSinTheta));

    // float p_u = 1.0 / (4.0 * PI);
    float inv_samples = 1.0 / float(SAMPLE_DIR * SAMPLE_DIR);

    vec3 L2 = vec3(0.0);
    vec3 f_ms = vec3(0.0);

    for (int i=0; i <= SAMPLE_DIR; i++ ) {
        for (int j=0; j <= SAMPLE_DIR; j++ ) {
            vec3 Lprime = vec3(0.0);
            vec3 Lf = vec3(0.0);

            single_scattering(x, sun_dir, float(i), float(j), Lprime, Lf);

            f_ms += inv_samples * Lf;
            L2 += inv_samples * Lprime;
        }
    }

    vec3 F_ms = 1.0 / (1.0 - f_ms); // Equation 9 : Infinite multiple scattering light transfer function.
    vec3 Psi_ms = L2 * F_ms; // Equation 10 : Total contribution = Second order scattering * Multiple scattering light
//    debugPrintfEXT("f_ms (%f %f %f), L2 (%f %f %f)", f_ms.x, f_ms.y, f_ms.z, L2.x, L2.y, L2.z);

    outFragColor = vec4(Psi_ms, 1.0);
}
