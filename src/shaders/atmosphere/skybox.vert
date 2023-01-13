#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vColor;
layout (location = 4) in vec4 vTangent;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
//layout (location = 3) out vec3 outFragPos; // fragment position
//layout (location = 4) out vec3 outCameraPos; // fragment position
//layout (location = 5) out vec4 outTangent;

layout(std140, set = 0, binding = 0) uniform  CameraBuffer
{
	mat4 view;
	mat4 proj;
	vec3 pos;
} cameraData;

void main() {
	mat4 modelView = cameraData.view;
	modelView[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	mat4 transformMatrix = (cameraData.proj * modelView);

	gl_Position = vec4(vPosition , 1.0f);

	outColor = vColor;
	outUV = vUV;
	outNormal = vNormal;
//	outTangent = vTangent;
//	outFragPos = vPosition;
//	outCameraPos = cameraData.pos;

	//outUVW = vPosition;
	// Convert cubemap coordinates into Vulkan coordinate space
	// outUVW.xy *= -1.0;
}
