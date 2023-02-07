#version 460

layout (set = 0, binding = 0) uniform sampler2D samplerMap; // 360 equirectangular map

layout (location = 0) in vec3 inPos;
layout (location = 0) out vec4 outFragColor;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 sample_spherical_map(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = sample_spherical_map(normalize(inPos));
    vec3 color = texture(samplerMap, uv).rgb;
    outFragColor = vec4(color, 1.0);
}