#version 460

layout (binding = 0) uniform samplerCube inputImage;

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec4 outFragColor;

#define PI 3.1415926535897932384626433832795

void main()
{
    vec3 N = normalize(inPos);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    const float TWO_PI = PI * 2.0;
    const float HALF_PI = PI * 0.5;

    vec3 irradiance = vec3(0.0);
    uint sampleCount = 0u;
    float deltaPhi = 0.025;
    float deltaTheta = 0.025;

    for (float phi = 0.0; phi < TWO_PI; phi += deltaPhi) {
        for (float theta = 0.0; theta < HALF_PI; theta += deltaTheta) {
            vec3 tempVec = cos(phi) * right + sin(phi) * up;
            vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
            irradiance += texture(inputImage, sampleVector).rgb * cos(theta) * sin(theta);
            sampleCount++;
        }
    }

    irradiance *= PI / float(sampleCount);

    outFragColor = vec4(irradiance, 1.0);
}
