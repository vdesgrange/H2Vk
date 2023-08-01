#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "./functions.glsl"

layout(set = 0, binding = 0) uniform sampler2D transmittanceLUT;
layout(set = 0, binding = 1) uniform sampler2D multiscatteringLUT;
layout(set = 0, binding = 2) uniform sampler2D skyviewLUT;

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

vec3 upscale_skyview(vec3 x, vec3 ray_dir, vec3 sun_dir) { // no upscale
    float height = length(x);
    vec3 up = x / height;

    vec3 right = cross(sun_dir, up);
    vec3 forward = cross(up, right);
    float azimuth = 0.0;
    float horizon = get_horizon_angle(height) + 0.5 * PI;
    float altitude = horizon - acos(dot(ray_dir, up));

    if (abs(altitude) <= (0.5 * PI - .0001)) {
        vec3 projected = normalize(ray_dir - up * dot(ray_dir, up));
        float sin_theta = dot(projected, right);
        float cos_theta = dot(projected, forward);
        azimuth = atan(sin_theta, cos_theta) + PI;
    }

    float v = 0.5 + 0.5 * sign(altitude) * sqrt(abs(altitude) * 2.0 / PI);
    vec2 uv = vec2(azimuth / (2.0 * PI), v);

    return texture(skyviewLUT, uv).rgb;
}

vec3 add_sun(vec3 ray_dir, vec3 sun_dir) {
    const float sun_solid_angle = 0.53 * PI / 180.0;
    const float min_sun_cos_theta = cos(sun_solid_angle);
    float cos_theta = dot(ray_dir, sun_dir);
    if (cos_theta >= min_sun_cos_theta) {
        return vec3(1.0);
    }

    float offset = min_sun_cos_theta - cos_theta;
    float gaussian_bloom = exp(-offset * 50000.0) * 0.5;
    float inv_bloom = 1.0 / (0.02 + offset * 300.0) * 0.01;

    return smoothstep(0.002, 1.0, vec3(gaussian_bloom + inv_bloom));
}

void main() {

    // mock data
    vec3 x = vec3(0.0, r_ground + 0.2, 0.0);
    vec3 cam_dir = normalize(vec3(0.0, 0.27, -1.0));
    float cam_fov_width = PI/3.5;
    float cam_width_scale = 2.0 * tan(cam_fov_width / 2.0);
    float cam_height_scale = cam_width_scale * float(ATMOSPHERE_RES.y) / float(ATMOSPHERE_RES.x);
    vec3 cam_right = normalize(cross(cam_dir, vec3(0.0, -1.0, 0.0)));
    vec3 cam_up = normalize(cross(cam_right, cam_dir));
    float sun_alti = PI / 32.0; // PI / 2.0 - acos(dot(tmp, up));

    // non-mock
    vec2 xy = 2.0 * uv.xy - 1.0;
    vec3 sun_dir =  normalize(vec3(0.0, sin(sun_alti), -cos(sun_alti)));
    vec3 ray_dir = normalize(cam_dir + cam_right * xy.x * cam_width_scale + cam_up * xy.y * cam_height_scale);

    vec3 L = upscale_skyview(x, ray_dir, sun_dir);

    L += add_sun(ray_dir, sun_dir);
    L *= 20.0;
    L = pow(L, vec3(1.3));
    L /= (smoothstep(0.0, 0.2, clamp(sun_dir.y, 0.0, 1.0))*2.0 + 0.15);
    L = pow(L, vec3(1.0/2.2));

    outFragColor = vec4(L, 1.0);
}