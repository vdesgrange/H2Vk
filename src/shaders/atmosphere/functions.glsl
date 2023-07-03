#version 460
#extension GL_EXT_debug_printf : disable

#include "./constants.glsl"

vec2 lutTransmittanceToUV(float r, float mu) {
  float rho = sqrt(max(0.0, r * r - r_ground * r_ground)); // distance to horizon
  float h = sqrt(r_top * r_top - r_ground * r_ground); // distance to top atmosphere boundary for horizontal ray

  float discriminant = r * r * (mu * mu - 1.0) + r_top * r_top;
  float d = -r * mu + max(0.0, sqrt(discriminant)); // distance to top atmosphere boundary
  float d_min = r_top - r;
  float d_max = rho + h;

  float u_mu = (d - d_min) / (d_max - d_min);
  float u_r = rho / h;
}


float getLayerDensity(float altitude, float e, float s, float l, float c) {
  float density = e * exp(s * altitude) + l * altitude + c;
  return clamp(density, 0.0, 1.0);
}

float getOpticalLength(DensityLayer profile, float r, float mu) {
  const SAMPLE_COUNT = 50; // Number of interval in numerical integration
  float discriminant = r * r * (mu * mu - 1.0) + r_top * r_top;
  float d = -r * mu + max(0.0, sqrt(discriminant)); // distance to top atmosphere boundary
  float dy = d / SAMPLE_COUNT; // Integration step

  // Numerical integration
  float result = 0.0;
  for (int i = 0; i <= SAMPLE_COUNT; i++) {
    float d_i = float(i) * dy; // 
    float r_i = sqrt(d_i * d_i+ 2.0 * r * mu * d_i + r * r); // distance between center and sample point
    float altitude = r_i - r_ground;
    float y_i = getLayerDensity(altitude, profile.e, profile.s, profile.l, profile.c); // compute layer density
    float w_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0; // boundary case

    result += w_i * y_i * dy;
  }

  return result;
}
