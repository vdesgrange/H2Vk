#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : enable

#include "./constants.glsl"

void get_beta_coefficients(vec3 x, out vec3 beta_s, out vec3 beta_a, out vec3 beta_e) {
  float altitude = length(x) - r_ground; // km
  float rayleigh_density = exp(-altitude / rayleight_scale); // e(-h/Hr)
  float mie_density = exp(-altitude / mie_scale); // e(-h/Hr)
  float ozone_density = max(0.0, 1.0 - abs(altitude - 25.0) / 15.0);

  vec3 beta_s_r = rayleigh_scattering * rayleigh_density;
  vec3 beta_a_r = rayleigh_absorption * rayleigh_density;
  vec3 beta_e_r = rayleigh_extinction * rayleigh_density;

  vec3 beta_s_m = mie_scattering * mie_density;
  vec3 beta_a_m = mie_absorption * mie_density;
  vec3 beta_e_m = mie_extinction * mie_density;

  vec3 beta_a_o = ozone_absorption * ozone_density;
  
  beta_s = beta_s_r + beta_s_m; // Scattering coefficient
  beta_a = beta_a_r + beta_a_m + beta_a_o; // Absorption coefficient
  beta_e = beta_s + beta_a;// Extinction coefficient
}

float get_rayleigh_phase(float mu) {
  float c = 3.0 / (16.0 * PI);
  return c * (1 + mu * mu); // mu = cos(theta)
}

float get_mie_phase(float g, float mu) {
  float c = 3.0 / (8.0 * PI);
  float num = (1 - g * g) * (1 + mu * mu);
  float den = (2 + g * g) * pow(1 + g * g - 2 * g * mu, 1.5);
  return c * num / den;
}

vec3 get_rayleigh_scattering_coefficient(vec3 x) {
  float altitude = length(x) - r_ground; // km
  float density = exp(-altitude / rayleight_scale); // e(-h/Hr)

  return rayleigh_scattering * density; // beta_s
}

vec3 get_rayleigh_in_scattering(vec3 x, float mu) {
  vec3 scattering = get_rayleigh_scattering_coefficient(x);
  float phase = get_rayleigh_phase(mu);

  return phase * scattering; // sigma_s_ray
}

vec3 get_mie_scattering_coefficient(vec3 x) {
  float altitude = length(x) - r_ground; // km
  float density = exp(-altitude / mie_scale); // e(-h/Hm)
  return mie_scattering * density; // beta_s
}

vec3 get_mie_in_scattering(vec3 x, float mu, float g) {
  vec3 scattering = get_mie_scattering_coefficient(x);
  float phase = get_mie_phase(g, mu);
  return phase * scattering; // sigma_s_mie
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
    float d_i = float(i) * dy;
    float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r); // distance between center and sample point
    float altitude = r_i - r_ground;

    float y_i = getProfileDensity(altitude, profiles); // compute layer density
    float w_i = i == 0 || i == SAMPLE_COUNT ? 0.5 : 1.0; // boundary case

    result += w_i * y_i * dy;
  }

  return result;
}

vec3 get_spherical_direction(float theta, float phi) {
     float cos_theta = cos(theta);
     float sin_theta = sin(theta);
     float cos_phi = cos(phi);
     float sin_phi = sin(phi);
     return vec3(sin_phi * sin_theta, cos_phi, sin_phi * cos_theta);
}

float get_ray_intersection_length(vec3 pos, vec3 dir, float radius) {
//  float b = dot(pos, dir);
//  float c = dot(pos, pos) - radius * radius;
//  if (c > 0.0f && b > 0.0) return -1.0;
//  float discr = b*b - c;
//  if (discr < 0.0) return -1.0;
//  // Special case: inside sphere, use far discriminant
//  if (discr > b*b) return (-b + sqrt(discr));
//  return -b - sqrt(discr);

  // Solution to hyperbolic curve at 0.
  float a = dot(dir, dir);
  float b = 2.0 * dot(pos, dir); // p.d - c, center considered as (0, 0, 0)
  float c = dot(pos, pos) - radius * radius;

  float delta = b * b - 4 * a * c;
  float solution_a = (-b + sqrt(delta)) / (2.0 * a);
  float solution_b = (-b - sqrt(delta)) / (2.0 * a);

  if (delta < 0.0 || a == 0) { // Curve does not cross axis or no quadratic curve
    return -1.0;
  }

  if (delta > b * b) {
    return solution_a;
  }

  return solution_b;
}


vec3 get_second_order_scattering() {
  vec3 L2 = vec3(0.0);
  const int SAMPLE_DIR = 8;
  const int SAMPLE_COUNT = SAMPLE_DIR * SAMPLE_DIR;

  for (int i=0; i <= SAMPLE_DIR; i++) {
    for (int j=0; j <= SAMPLE_DIR; j++) {
      float theta = PI * (float(i) + 0.5) / float(SAMPLE_DIR);
      float phi = clamp(acos(1.0 - 2.0*(float(j) + 0.5) / float(SAMPLE_DIR)), -1.0, 1.0);
      vec3 ray_dir = get_spherical_direction(theta, phi);
    }
  }

  return L2;
}

vec2 get_uv_for_TLUT(vec3 x, vec3 sun_dir) {
  float h = length(x);
  vec3 up = x / h;
  float cosThetaSun = dot(sun_dir, up);
  vec2 uv = vec2(
    clamp(0.5 + 0.5 * cosThetaSun, 0.0, 1.0),
    max(0.0, min((h - r_ground) / (r_top - r_ground), 1.0))
  ); // * TRANSMITTANCE_LUT_RES / MULTISCATTER_LUT_RES

  return uv;
}

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

