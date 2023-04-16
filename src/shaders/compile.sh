#!/bin/bash

glslc mesh/scene.frag -o mesh/scene.frag.spv
glslc mesh/mesh.vert -o mesh/mesh.vert.spv
glslc mesh/mesh_tex.vert -o mesh/mesh_tex.vert.spv
glslc mesh/scene_tex.frag -o mesh/scene_tex.frag.spv
glslc light/light.vert -o light/light.vert.spv
glslc light/light.frag -o light/light.frag.spv
glslc skybox/skybox.vert -o skybox/skybox.vert.spv
glslc skybox/skybox.frag -o skybox/skybox.frag.spv
glslc skybox/skysphere.vert -o skybox/skysphere.vert.spv
glslc skybox/skysphere.frag -o skybox/skysphere.frag.spv
glslc pbr/pbr.vert -o pbr/pbr.vert.spv
glslc pbr/pbr.frag -o pbr/pbr.frag.spv
glslc pbr/pbr_tex.frag -o pbr/pbr_tex.frag.spv
glslc pbr/pbr_ibl_cube.frag -o pbr/pbr_ibl_cube.frag.spv
glslc pbr/pbr_tex_cube.frag -o pbr/pbr_tex_cube.frag.spv
glslc env_map/cube_map.vert -o env_map/cube_map.vert.spv
glslc env_map/converter.frag -o env_map/converter.frag.spv
glslc env_map/irradiance.frag -o env_map/irradiance.frag.spv
glslc env_map/irradiance.comp -o env_map/irradiance.comp.spv
glslc env_map/brdf.comp -o env_map/brdf.comp.spv
glslc shadow_map/offscreen.vert -o shadow_map/offscreen.vert.spv
glslc shadow_map/offscreen.frag -o shadow_map/offscreen.frag.spv
glslc shadow_map/quad.vert -o shadow_map/quad.vert.spv
glslc shadow_map/quad.frag -o shadow_map/quad.frag.spv
