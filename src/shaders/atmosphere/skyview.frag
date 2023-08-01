#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "./functions.glsl"

layout(set = 0, binding = 0) uniform sampler2D transmittanceLUT;
layout(set = 0, binding = 1) uniform sampler2D multiscatteringLUT;

//layout(std140, set = 0, binding = 0) uniform  CameraBuffer
//{
//    mat4 view;
//    mat4 proj;
//    vec3 pos;
//    bool flip;
//} cameraData;
//
//layout (push_constant) uniform NodeModel {
//    mat4 model;
//} nodeData;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

/**
* Get altitude angle [-pi/2, pi/2] from v [0,1] with non-linear mapping
* h is horizon angle
*/
float get_altitude_angle(float v, float h) {
    // Section 5.3
    // non-linear mapping g:[0,1] -> [-pi/2, pi/2]
    float den = 0.0;
    float V = 2.0 * v - 1.0;
    if (v < 0.5) {
        V *= -1;
        den = - V * V;
    } else {
        den = V * V;
    }

    return PI / 2.0 * den - h;
}

vec3 get_light_scattering(vec3 x, vec3 ray_dir, vec3 sun_dir) {
    const int N_LIGHT = 30;
    const float offset = 0.3;

    float t_atmo = get_ray_intersection_length(x, ray_dir, r_top); // / 1000.0
    float t_ground = get_ray_intersection_length(x, ray_dir, r_ground); //  / 1000.0
    float t_max = t_atmo;
    if (t_ground > 0.0) {
        t_max = t_ground;
    }

    float nu = dot(sun_dir, ray_dir); // nu, cosinus of angle between sun and ray direction
    float p_r = get_rayleigh_phase(-nu);
    float p_m = get_mie_phase(0.8, nu);

    vec3 L = vec3(0.0);
    vec3 T = vec3 (1.0);
    float t = 0.0;
    float dt = t_max / N_LIGHT;

    for (int i=0; i < N_LIGHT; i++) {
        float new_t = ((i + offset) / N_LIGHT) * t_max;
        dt = new_t - t;
        t = new_t;

        vec3 x_tv = x + t * ray_dir;

        vec3 beta_s = vec3(0.0);
        vec3 beta_a = vec3(0.0);
        vec3 beta_e = vec3(0.0);
        get_beta_coefficients(x_tv, beta_s, beta_a, beta_e);

        vec3 sample_T = exp(-dt * beta_e);

        vec2 sun_uv = get_uv_for_LUT(x_tv, sun_dir);
        vec3 T_sun = texture(transmittanceLUT, sun_uv).xyz; // or lutTransmittanceToUV
        if (isnan(T_sun.x) || isnan(T_sun.y) || isnan(T_sun.z)) {
            debugPrintfEXT("T_sun (%f %f %f) i %i dt %f", T_sun.x, T_sun.y, T_sun.z, i, dt);
        }

        vec3 Psi_ms = texture(multiscatteringLUT, sun_uv).xyz;

        vec3 beta_s_r = get_rayleigh_scattering_coefficient(x_tv);
        vec3 sigma_s_r = beta_s_r * (vec3(p_r) * T_sun + Psi_ms); // rayleight scattering * density * phase function
        vec3 beta_s_m = get_mie_scattering_coefficient(x_tv);
        vec3 sigma_s_m = beta_s_m * (vec3(p_m) * T_sun + Psi_ms); // mie scattering * density * phase function
        vec3 sigma_s = sigma_s_r + sigma_s_m;
        if (isnan(sigma_s.x) || isnan(sigma_s.y) || isnan(sigma_s.z)) {
            debugPrintfEXT("x_tv (%f %f %f)", x_tv.x, x_tv.y, x_tv.z);
        }

        vec3 integral_sigma_s = (sigma_s - sigma_s * sample_T) / beta_e;

        L += integral_sigma_s * T;
        if (integral_sigma_s.x == 0 || integral_sigma_s.y == 0 || integral_sigma_s.z == 0) {
            debugPrintfEXT("sigma_s (%f %f %f)", sigma_s.x, sigma_s.y, sigma_s.z);
        }

        T *= sample_T;
    }

    return L;
}

void main() {
    vec3 x = vec3(0.0, r_ground + 0.2, 0.0); // rayon : km ou m?
    float height = length(x);

    float azimuth = 2 * PI * (uv.x - 0.5); // linear mapping f:[0, 1] -> [-pi, pi]
    float horizon = get_horizon_angle(height);
    float altitude = get_altitude_angle(uv.y, horizon);

    vec3 up = x / height;
    float sun_alti = PI / 32.0; // PI / 2.0 - acos(dot(tmp, up));
    vec3 sun_dir =  normalize(vec3(0.0, sin(sun_alti), -cos(sun_alti)));
    vec3 ray_dir = vec3(cos(altitude) * sin(azimuth), sin(altitude), -cos(altitude) * cos(azimuth)); // get_spherical_direction(altitude, azimuth); ?

    vec3 L = get_light_scattering(x, ray_dir, sun_dir);

    outFragColor = vec4(L, 1.0);
}
