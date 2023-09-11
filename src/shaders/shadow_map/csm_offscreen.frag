#version 460

// layout (set = 2, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{	
    // No shadow if transparency
//	float alpha = texture(colorMap, inUV).a;
//	if (alpha < 0.5) {
//		discard;
//	}

	outFragColor = vec4(inUV, 0.0, 1.0);
}