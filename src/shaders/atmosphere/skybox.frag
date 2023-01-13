#version 460

layout (set = 0, binding = 1) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
//layout (location = 3) in vec3 inFragPos; // fragment position
//layout (location = 4) in vec3 inCameraPos; // camera/view position

layout (location = 0) out vec4 outFragColor;

void main() {
	// outFragColor = texture(samplerCubeMap, inUVW);
	outFragColor = vec4(inNormal.xyz, 1.0);

}
