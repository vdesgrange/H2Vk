#version 450

layout (set = 2, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec2 inUV;

void main() 
{	
    // No shadow if transparency : BUG - segfault if use with non-pbr shader
	float alpha = texture(colorMap, inUV).a;
	if (alpha < 0.5) {
		discard;
	}

//	gl_FragDepth = 1.0;
}