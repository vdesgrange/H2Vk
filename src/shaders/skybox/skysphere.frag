#version 460

layout (set = 0, binding = 1) uniform sampler2D samplerMap;

layout (location = 0) in vec2 inUV; // fragment position

layout (location = 0) out vec4 outFragColor;

void main() {
    outFragColor = texture(samplerMap, inUV);
}
