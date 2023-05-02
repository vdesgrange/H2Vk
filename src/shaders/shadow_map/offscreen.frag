#version 450
#extension GL_EXT_debug_printf : disable

layout (location = 0) in vec2 inUV;

layout(depth_any) out float gl_FragDepth;

void main() {
//     outFragColor = vec4(inUV.x, 0.5, 0.5, 1.0);
    gl_FragDepth = 0.5f;
}