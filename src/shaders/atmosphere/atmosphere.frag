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

vec2 getValFromSkyLUT(vec3 x, vec3 rayDir, vec3 sunDir) {
    float height = length(x);
    vec3 up = x / height;

    float horizon = acos(sqrt((height * height) - (r_ground * r_ground)) / height);
    float altitudeAngle = horizon - acos(dot(rayDir, up)); // Between -PI/2 and PI/2
    float azimuthAngle; // Between 0 and 2*PI
    if (abs(altitudeAngle) > (0.5*PI - .0001)) {
        azimuthAngle = 0.0;
    } else {
        vec3 right = cross(sunDir, up);
        vec3 forward = cross(up, right);

        vec3 projectedDir = normalize(rayDir - up*(dot(rayDir, up)));
        float sinTheta = dot(projectedDir, right);
        float cosTheta = dot(projectedDir, forward);
        azimuthAngle = atan(sinTheta, cosTheta) + PI;
    }

    // Non-linear mapping of altitude angle. See Section 5.3 of the paper.
    float v = 0.5 + 0.5*sign(altitudeAngle)*sqrt(abs(altitudeAngle)*2.0/PI);
    vec2 tmpUV = vec2(azimuthAngle / (2.0*PI), v);

    return tmpUV;
}

vec2 skyViewLutParamsToUV(bool intersectGround, float viewZenithCosAngle, float lightViewCosAngle, float height) {
    vec2 tmpUV;
    float horizon = acos(sqrt((height * height) - (r_ground * r_ground)) / height);
    float zenithHorizonAngle = PI - horizon;

    if (!intersectGround) {
        float coord = acos(viewZenithCosAngle) / zenithHorizonAngle;
        coord = 1.0 - coord;
        coord = 1.0 - coord;
        tmpUV.y = coord * 0.5;
    }
    else {
        float coord = (acos(viewZenithCosAngle) - zenithHorizonAngle) / horizon;
        tmpUV.y = coord * 0.5 + 0.5;
    }

    float coord = -lightViewCosAngle * 0.5 + 0.5;
    coord = sqrt(coord);
    tmpUV.x = coord;

    return vec2(tmpUV.x + 0.01, tmpUV.y + 0.01);
}

vec3 add_sun(vec3 x, vec3 ray_dir, vec3 sun_dir) {
    const float d = 0.545 * PI / 180.0; // sun angular diameter - solid angle = PI / 4 * d^2
    const float min_sun_cos_theta = cos(d);
    float cos_theta = dot(ray_dir, sun_dir);

    if (cos_theta >= min_sun_cos_theta) { // sun in field of view
        // debugPrintfEXT("Flip %i Ray %f %f %f Sun : %f %f %f", cameraData.flip, ray_dir.x, ray_dir.y, ray_dir.z, sun_dir.x, sun_dir.y, sun_dir.z);
        return vec3(sun_dir);// Debug sun position
    }

    float offset = min_sun_cos_theta - cos_theta;
    float gaussian_bloom = exp(-offset * 50000.0) * 0.5;
    float inv_bloom = 1.0 / (0.02 + offset * 300.0) * 0.01;
    return vec3(gaussian_bloom + inv_bloom);

//    vec3 sun_l = smoothstep(0.002, 1.0, vec3(gaussian_bloom + inv_bloom));
//    if (length(sun_l) > 0.0) {
//        if (get_ray_intersection_length(x, ray_dir, r_ground) >= 0.0) { // sun-camera stopped by ground
//            sun_l *= 0.0;
//        }
//    }

//    return sun_l;

}


void main() {
    vec2 cs = 2.0 * uv.xy - 1.0;
    vec3 sun_direction = normalize(pushData.sun_direction.xyz); // wip
    mat4 view_proj = inverse(cameraData.proj * cameraData.view);
    vec4 h_pos = view_proj * vec4(cs, 1.0, 1.0);
    vec3 dir = normalize(h_pos.xyz / h_pos.w - cameraData.pos); // normalize(vec3(0.0, 0.27, -1.0));
    vec3 x =  cameraData.pos + vec3(0.0, r_ground, 0.0);

    float height = length(x);
    vec3 up = x / height;
    float sun_alti = acos(dot(up, sun_direction));
    vec3 sun_dir = normalize(vec3(0.0, -cos(sun_alti), -sin(sun_alti))); // normalize(vec3(0.0, sin(sun_alti), -cos(sun_alti))); //

    float f_near = 2.0;
    float cam_fov_width = 0.38;
    float cam_width_scale = f_near * tan(cam_fov_width / 2.0);
    float cam_height_scale = cam_width_scale * float(ATMOSPHERE_RES.y) / float(ATMOSPHERE_RES.x);
    vec3 cam_right = normalize(cross(dir, vec3(0.0, 1.0, 0.0)));
    vec3 cam_up = normalize(cross(cam_right, dir));
    vec3 ray_dir = normalize(dir + cam_right * cs.x * cam_width_scale + cam_up * cs.y * cam_height_scale);


    vec3 l = vec3(0.0);
    l += add_sun(x, dir, sun_dir); // ray_dir

    if (height < r_top) {
        vec3 up = normalize(x); // vec3(0.0, 1.0, 0.0);
        float viewZenithCosAngle = dot(dir, up);

        vec3 right = normalize(cross(up, dir));
        vec3 forward = normalize(cross(right, up)); // dir
        vec2 lightOnPlane = normalize(vec2(dot(sun_dir, forward), dot(sun_dir, right)));
        float lightViewCosAngle = lightOnPlane.x;

        bool intersectGround = get_ray_intersection_length(x, dir, r_ground) >= 0.0;
        vec2 tmpUV = skyViewLutParamsToUV(intersectGround, viewZenithCosAngle, lightViewCosAngle, height);
        // vec2 tmpUV = getValFromSkyLUT(x, ray_dir, sun_dir);

        l += texture(skyviewLUT, tmpUV).rgb;

        outFragColor = vec4(l, 1.0);
    } else {
        outFragColor = vec4(0.0, uv.xy, 1.0);
    }
}
