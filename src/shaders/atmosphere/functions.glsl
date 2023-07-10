#extension GL_EXT_debug_printf : disable
#extension GL_GOOGLE_include_directive : enable

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

  return vec2(u_mu, u_r);
}


float getLayerDensity(float altitude, float e, float s, float l, float c) {
  float density = e * exp(s * altitude) + l * altitude + c;
  return clamp(density, 0.0, 1.0);
}

float getProfileDensity(float altitude, DensityProfile profiles) {
  return (altitude < profiles.layers[0].w) ?
    getLayerDensity(altitude, profiles.layers[0].e, profiles.layers[0].s, profiles.layers[0].l, profiles.layers[0].c) :
    getLayerDensity(altitude, profiles.layers[1].e, profiles.layers[1].s, profiles.layers[1].l, profiles.layers[1].c);
}

float getOpticalLength(DensityProfile profiles, float r, float mu) {
  const int SAMPLE_COUNT = 50; // Number of interval in numerical integration
  float discriminant = r * r * (mu * mu - 1.0) + r_top * r_top;
  float d = -r * mu + max(0.0, sqrt(discriminant)); // distance to top atmosphere boundary
  float dy = d / SAMPLE_COUNT; // Integration step

  // Numerical integration
  float result = 0.0;
  for (int i = 0; i <= SAMPLE_COUNT; i++) {
    float d_i = float(i) * dy; // 
    float r_i = sqrt(d_i * d_i+ 2.0 * r * mu * d_i + r * r); // distance between center and sample point
    float altitude = r_i - r_ground;

    float y_i = getProfileDensity(altitude, profiles); // compute layer density
    float w_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0; // boundary case

    result += w_i * y_i * dy;
  }

  return result;
}
