#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "./functions.glsl"

layout(set = 0, binding = 0) uniform sampler2D transmittanceLUT;
layout(set = 0, binding = 1) uniform sampler2D multiscatteringLUT;

layout(std140, set = 0, binding = 2) uniform  CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec3 pos;
    bool flip;
} cameraData;

layout (push_constant) uniform PushConstants {
    vec4 sun_direction;
} pushData;

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
    if (v < 0.5) {
        float V = 1.0 - 2.0 * v;
        den = - V * V;
    } else {
        float V = 2.0 * v - 1.0;
        den = V * V;
    }

    return PI / 2.0 * den - h;
}

vec2 get_angles(vec2 uv, float h) {
    float azimuth = 2.0 * PI * (0.5 - uv.x); // linear mapping f:[0, 1] -> [-pi, pi]
    float horizon = get_horizon_angle(h);
    float altitude = get_altitude_angle(uv.y, horizon);

    return vec2(altitude, azimuth);
}

vec2 get_angles2(vec2 uv, float h) {
    float horizon = acos(sqrt((h * h) - (r_ground * r_ground)) / h);
    float zenithHorizonAngle = PI - horizon;
    float viewZenithCosAngle;

    if (uv.y < 0.5) {
        float coord = uv.y * 2.0;
        viewZenithCosAngle = zenithHorizonAngle * coord;
    }
    else {
        float coord = uv.y * 2.0 - 1.0;
        viewZenithCosAngle = zenithHorizonAngle + horizon * coord;
    }

    float lightViewCosAngle = 1.0 - 2.0 * uv.x * uv.x;

    return vec2(viewZenithCosAngle, acos(lightViewCosAngle));
}

vec3 get_light_scattering(vec3 x, vec3 ray_dir, vec3 sun_dir) {
    const float N_LIGHT = 32.0;
    const float offset = 0.3;

    float t_atmo = get_ray_intersection_length(x, ray_dir, r_top);
    float t_ground = get_ray_intersection_length(x, ray_dir, r_ground);
    float t_max = t_atmo;
    if (t_ground >= 0.0) {
        t_max = t_ground;
    }

    float nu = dot(sun_dir, ray_dir); // nu, cosinus of angle between sun and ray direction
    float p_r = get_rayleigh_phase(-nu);
    float p_m = get_mie_phase(0.8, nu);

    vec3 L = vec3(0.0); // Sky color value
    vec3 T = vec3 (1.0);
    float t = 0.0;
    float dt = 0.0;

    for (float i=0.0; i < N_LIGHT; i += 1.0) {
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
        vec3 T_sun = texture(transmittanceLUT, sun_uv).xyz; // transmittance (absorption)

        vec3 Psi_ms = texture(multiscatteringLUT, sun_uv).xyz; // multiple scattering

        vec3 beta_s_r = get_rayleigh_scattering_coefficient(x_tv); // Scattering rayleigh
        vec3 beta_s_m = get_mie_scattering_coefficient(x_tv); // Scattering Mie
        vec3 sigma_s2 = beta_s_r * vec3(p_r) + beta_s_m * vec3(p_m);

        vec3 sigma_s_r = beta_s_r * (vec3(p_r) * T_sun + Psi_ms); // rayleight scattering * density * phase function
        vec3 sigma_s_m = beta_s_m * (vec3(p_m) * T_sun + Psi_ms); // mie scattering * density * phase function
        vec3 sigma_s = sigma_s_r + sigma_s_m; // in scattering
        vec3 integral_sigma_s = (sigma_s - sigma_s * sample_T) / max(beta_e, 0.00001f);

        L += integral_sigma_s * T;

        // Version 2: Psi_ms not in integration
        // vec3 scatter_integ = (sigma_s2 - sigma_s2 * sample_T) / max(beta_e, 0.00001f);
        // L += T_sun * scatter_integ;
        // L += Psi_ms * (beta_s_r + beta_s_m) * dt * T;


        T *= sample_T;
    }

    return L;
}

void main() {
    vec2 xy = 2.0 * uv.xy - 1.0;
    vec3 sun_direction = normalize(pushData.sun_direction.xyz);
    mat4 view_proj = inverse(cameraData.proj * cameraData.view);
    vec4 h_pos = view_proj * vec4(xy, 1.0, 1.0);
    vec3 dir = normalize(h_pos.xyz / h_pos.w - cameraData.pos);
    vec3 x = cameraData.pos + vec3(0.0, r_ground + 0.2, 0.0);

    float height = length(x);

    float azimuth = 2.0 * PI * (0.5 - uv.x); // linear mapping f:[0, 1] -> [-pi, pi]
    float horizon = get_horizon_angle(height);
    float altitude = get_altitude_angle(uv.y, horizon);

    vec3 up = x / height;
    float sun_alti = PI / 2.0 - acos(dot(up, sun_direction));
//    debugPrintfEXT("Sun direction %f %f %f altitude : %f", sun_direction.x, sun_direction.y, sun_direction.z, sun_alti);
    vec3 sun_dir = normalize(vec3(0.0, sin(sun_alti), -cos(sun_alti))); // normalize(vec3(sin(sun_alti), cos(sun_alti), 0.0));

    vec3 ray_dir = vec3(cos(altitude) * sin(azimuth), sin(altitude), cos(altitude) * cos(azimuth));

//    vec2 skyViewLutParams = get_angles2(uv, height);
//    skyViewLutParams = get_angles(uv, height);
//    float viewZenithAngle = skyViewLutParams.x;
//    float lightViewAngle = skyViewLutParams.y;
//    ray_dir = vec3(cos(viewZenithAngle) * sin(lightViewAngle), sin(viewZenithAngle), cos(viewZenithAngle) * cos(lightViewAngle));
    // ray_dir = vec3(sin(viewZenithAngle) * cos(lightViewAngle), cos(viewZenithAngle), sin(viewZenithAngle) * sin(lightViewAngle));

    vec3 L = get_light_scattering(x, ray_dir, sun_dir);

    outFragColor = vec4(L, 1.0);
}