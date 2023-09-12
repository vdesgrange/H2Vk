#version 450
// #extension GL_EXT_debug_printf : enable
// layout (set = 2, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec2 inUV;

void main() 
{	
    // No shadow if transparency
//	float alpha = texture(colorMap, inUV).a;
//	if (alpha < 0.5) {
//		discard;
//	}

	gl_FragDepth = 0.5;
}