
struct DensityLayer {
  float e; // exponential term
  float s; // scale term
  float l; // linear term
  float c; // constant term
  float w;
};

struct DensityProfile {
  DensityLayer layers[2];
};

const float PI = 3.14159265358979323846f;

const vec3 albedo = vec3(0.3);
const float r_ground = 6360.0; // 6360.0; // m
const float r_top = 6420.0; // 6420.0;

const vec2 TRANSMITTANCE_LUT_RES = vec2(256.0, 64.0);
const vec2 MULTISCATTER_LUT_RES = vec2(32.0, 32.0);
const vec3 AERIAL_LUT_RES = vec3(32.0, 32.0, 32.0);
const vec2 SKYVIEW_LUT_RES = vec2(1200.0, 800.0);
const vec2 ATMOSPHERE_RES = vec2(1200.0, 800.0); // todo : inject the resolution constants into shader

// --- Transmittance ---
//
// Rayleight
const float rayleight_scale = 8.0; // km
const vec3 rayleigh_scattering = vec3(0.0058, 0.0135, 0.0331); // m^-1 / * 1e3 for km-1
const vec3 rayleigh_extinction = rayleigh_scattering;
const vec3 rayleigh_absorption = vec3(0.0);
DensityProfile rayleigh_density = DensityProfile(DensityLayer[2](
  DensityLayer(0.0, 0.0, 0.0, 0.0, 0.0),
  DensityLayer(1.0, -1.0 / rayleight_scale, 0.0, 0.0, 0.0)// altitude density distribution
));

// Mie
const float mie_scale = 1.2; // km
const vec3 mie_scattering = vec3(0.003996, 0.003996, 0.003996);
const vec3 mie_extinction = vec3(0.004440, 0.004440, 0.004440); // mie_scattering / 0.9
const vec3 mie_absorption = vec3(0.004440, 0.004440, 0.004440);
DensityProfile mie_density = DensityProfile(DensityLayer[2](
  DensityLayer(0.0, 0.0, 0.0, 0.0, 0.0),
  DensityLayer(1.0, -1.0 / mie_scale, 0.0, 0.0, 0.0)
)); // altitude density distribution

// Ozone (absorption layer)
const vec3 ozone_scattering = vec3(0.0);
const vec3 ozone_extinction = vec3(0.0);
const vec3 ozone_absorption = vec3(0.00065, 0.001881, 0.000085);
DensityProfile ozone_density = DensityProfile(DensityLayer[2](
  DensityLayer(0.0, 0.0, 1.0 / 15.0, -2.0 / 3.0, 25.0),
  DensityLayer(0.0, 0.0, -1.0 / 15.0, 8.0 / 3.0, 0.0)
)); // altitude density distribution

const vec3 ground_albedo = vec3(0.0, 0.0, 0.0);

