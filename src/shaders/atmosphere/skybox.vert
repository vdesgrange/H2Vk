#version 460

// layout (location = 0) in vec3 inPos;
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

//layout (binding = 0) uniform UBO {
//	mat4 projection;
//	mat4 model;
//} ubo;

layout(set = 0, binding = 0) uniform  CameraBuffer
{
	mat4 view;
	mat4 proj;
	vec3 pos;
} cameraData;

layout (location = 0) out vec3 outUVW;

void main() {
	// gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
	mat4 transformMatrix = (cameraData.proj * cameraData.view *  modelMatrix);
	gl_Position = transformMatrix * vec4(vPosition.xyz , 1.0f);
	outUVW = vPosition;
	// Convert cubemap coordinates into Vulkan coordinate space
	// outUVW.xy *= -1.0;
}
