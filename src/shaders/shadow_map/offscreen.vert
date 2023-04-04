#version 460

layout (location = 0) in vec3 vPosition;

layout (binding = 0) uniform UBO {
    mat4 depthMVP;
} ubo;

void main() {
    gl_Position =  ubo.depthMVP * vec4(vPosition, 1.0);
}