#version 460
#extension GL_EXT_debug_printf : disable

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

  float u_mu = (d == 0.0) * m ? 1.0 : (h * h - rho * rho - d * d) / (2.0 * r * d);
  u_mu = clamp(u_mu, -1.0, 1.0);

  float rayleigh_density = 0.0;
  float mie_density = 0.0;
  float absorption_density = 0.0;
  
  vec3 transmittance = exp(-(
    rayleigh_extinction * getOpticalLength(rayleigh_density, r, mu) + 
    mie_extinction * getOpticalLength(mie_density, r, mu) +
    ozone_extinction * getOpticalLength(ozone_density, r, mu))
  );

  outFragColor = vec4(transmittance, 1.0);
}
