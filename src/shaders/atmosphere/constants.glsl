
struct DensityLayer {
  float e; // exponential term
  float s; // scale term
  float l; // linear term
  float c; // constant term
}

const float PI = 3.14159265358979323846f;

const r_ground = 6360; // km
const r_top = 6420;

const float rayleight_scale = 8.0;
const vec3 rayleigh_scattering = vec3(0.0000058, 0.0000135, 0.0000331); // m^-1
const vec3 rayleigh_extinction = - rayleigh_scattering;
const vec3 rayleigh_absorption = vec3(0.0);
DensityLayer rayleigh_density = DensityLayer(); //  to configure

const float mie_scale = 1.2; // km
const vec3 mie_scattering = vec3(0.00004, 0.00004, 0.00004);
const vec3 mie_extinction = mie_scattering / 0.9;
const vec3 mie_absorption = mie_extinction - mie_scattering;
DensityLayer mie_density = DensityLayer(); //  to configure

const vec3 ozone_scattering = vec3(0.0);
const vec3 ozone_extinction = vec3(0.0);
const vec3 ozone_absorption = vec3(0.0000065, 0.0000018, 0.00000008);
DensityLayer ozone_density = DensityLayer(); //  to configure

const vec3 ground_albedo = vec3(0.0, 0.0, 0.0);

