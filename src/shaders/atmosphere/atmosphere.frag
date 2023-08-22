#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "./functions.glsl"

layout(set = 0, binding = 0) uniform sampler2D transmittanceLUT;
layout(set = 0, binding = 1) uniform sampler2D multiscatteringLUT;
layout(set = 0, binding = 2) uniform sampler2D skyviewLUT;

layout(std140, set = 0, binding = 3) uniform  CameraBuffer
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

vec2 get_skyview_uv(vec3 x, vec3 dir, vec3 sun_dir) {
    float height = length(x);
    vec3 up = x / height;

    float horizon = acos(sqrt((height * height) - (r_ground * r_ground)) / height);
    float altitude = horizon - acos(dot(dir, up)); // Between -PI/2 and PI/2
    float azimuth; // Between 0 and 2*PI

    if (abs(altitude) > (0.5 * PI - .0001)) {
        azimuth = 0.0;
    } else {
        vec3 right = cross(sun_dir, up);
        vec3 forward = cross(up, right);

        vec3 proj_dir = normalize(dir - up*(dot(dir, up)));
        float sin_theta = dot(proj_dir, right);
        float cos_theta = dot(proj_dir, forward);
        azimuth = atan(sin_theta, cos_theta) + PI;
    }

    // Non-linear mapping of altitude angle. See Section 5.3 of the paper.
    float v = 0.5 + 0.5 * sign(altitude) * sqrt(abs(altitude) * 2.0 / PI);
    vec2 tmpUV = vec2(azimuth / (2.0 * PI), 1.0 - v); // (1.0 - v) instead of v ?

    return tmpUV;
}

vec3 add_sun(vec3 x, vec3 ray_dir, vec3 sun_dir) {
    const float d = 0.545 * PI / 180.0; // sun angular diameter - solid angle = PI / 4 * d^2
    const float min_sun_cos_theta = cos(d);
    float cos_theta = dot(ray_dir, sun_dir);
    vec3 sun_l = vec3(0.0);

    if (cos_theta >= min_sun_cos_theta) { // sun in field of view
        sun_l = vec3(1.0); // vec3(sun_dir) Debug sun position
    } else {
        float offset = min_sun_cos_theta - cos_theta;
        float gaussian_bloom = exp(-offset * 50000.0) * 0.5;
        float inv_bloom = 1.0 / (0.02 + offset * 300.0) * 0.01;
        sun_l = vec3(gaussian_bloom + inv_bloom);
    }

    sun_l = smoothstep(0.002, 1.0, sun_l);
    if (length(sun_l) > 0.0) {
        if (get_ray_intersection_length(x, ray_dir, r_ground) <= 0.0) { // sun-camera stopped by ground
            sun_l *= 0.0;
        }
//        else {
//            vec2 tmpUV = get_uv_for_LUT(x, sun_dir);
//            sun_l *= texture(transmittanceLUT, tmpUV).rgb;
//        }
    }

    return sun_l;

}


void main() {
    float flip = cameraData.flip ? -1.0 : 1.0;
    vec2 cs = vec2(2.0, flip * 2.0) * uv.xy - vec2(1.0, flip);
    vec3 sun_direction = normalize(pushData.sun_direction.xyz);
    mat4 view_proj = inverse(cameraData.proj * cameraData.view);
    vec4 h_pos = view_proj * vec4(cs, 1.0, 1.0);
    vec3 dir = normalize(h_pos.xyz / h_pos.w - cameraData.pos);
    vec3 x =  cameraData.pos + vec3(0.0, r_ground, 0.0);

    float height = length(x);
    vec3 up = x / height;
    float sun_alti = acos(dot(up, sun_direction));
    vec3 sun_dir = normalize(vec3(0.0, - cos(sun_alti), -sin(sun_alti)));

    vec3 l = vec3(0.0);
    x = vec3(0.0, height, 0.0); // should we only consider altitude?
    // l += add_sun(x, dir, sun_dir); // WIP

    if (height < r_top) {
        vec3 up = normalize(x);

        vec3 right = normalize(cross(up, dir));
        vec3 forward = normalize(cross(right, up)); // dir
        vec2 lightOnPlane = normalize(vec2(dot(sun_dir, forward), dot(sun_dir, right)));
        float lightViewCosAngle = lightOnPlane.x;

        vec2 tmpUV = get_skyview_uv(x, dir, sun_dir);

        //bool intersectGround = get_ray_intersection_length(x, dir, r_ground) >= 0.0;

        l += texture(skyviewLUT, tmpUV).rgb;

        outFragColor = vec4(l, 1.0);
    } else {
        outFragColor = vec4(0.0, uv.xy, 1.0);
    }
}
