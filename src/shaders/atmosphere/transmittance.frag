#version 460
#extension GL_EXT_debug_printf : disable
#extension GL_GOOGLE_include_directive : enable
// #extension GL_ARB_shading_language_include : enable

#include "./functions.glsl"

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

void main() {
  float x_mu = uv.x;
  float x_r = uv.y;
  
  float h = sqrt(r_top * r_top - r_ground * r_ground); // d_H distance to atmosphere boundary for horizontal ray
  float rho = h * x_r; // distance to horizon
  float r = sqrt(rho * rho + r_ground * r_ground);
  // distance to top atmosphere boundary for a ray (r, mu)
  float d_min = r_top - r; // minimum
  float d_max = rho + h; // maximum
  float d = d_min + x_mu * (d_max - d_min); // actual distance

  float mu = (d == 0.0) ? 1.0 : (h * h - rho * rho - d * d) / (2.0 * r * d);
  mu = clamp(mu, -1.0, 1.0);
  
  vec3 transmittance = exp(-(
    rayleigh_extinction * getOpticalLength(rayleigh_density, r, mu) + 
    mie_extinction * getOpticalLength(mie_density, r, mu) +
    ozone_extinction * getOpticalLength(ozone_density, r, mu))
  );

  outFragColor = vec4(transmittance, 1.0);
  // outFragColor = vec4(uv.x, uv.y, 0.0, 1.0);
}
