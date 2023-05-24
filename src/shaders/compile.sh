#!/bin/bash

glslc skybox/skybox.vert -o skybox/skybox.vert.spv
glslc skybox/skybox.frag -o skybox/skybox.frag.spv
glslc skybox/skysphere.vert -o skybox/skysphere.vert.spv
glslc skybox/skysphere.frag -o skybox/skysphere.frag.spv
glslc pbr/pbr_ibl.vert -o pbr/pbr_ibl.vert.spv
glslc pbr/pbr_ibl.frag -o pbr/pbr_ibl.frag.spv
glslc pbr/pbr_ibl_tex.vert -o pbr/pbr_ibl_tex.vert.spv
glslc pbr/pbr_ibl_tex.frag -o pbr/pbr_ibl_tex.frag.spv
glslc env_map/cube_map.vert -o env_map/cube_map.vert.spv
glslc env_map/converter.frag -o env_map/converter.frag.spv
glslc env_map/irradiance.frag -o env_map/irradiance.frag.spv
glslc env_map/prefilter.frag -o env_map/prefilter.frag.spv
glslc env_map/brdf.comp -o env_map/brdf.comp.spv
glslc shadow_map/offscreen.vert -o shadow_map/offscreen.vert.spv
glslc shadow_map/depth_debug_quad.vert -o shadow_map/depth_debug_quad.vert.spv
glslc shadow_map/depth_debug_quad.frag -o shadow_map/depth_debug_quad.frag.spv
glslc shadow_map/depth_debug_scene.vert -o shadow_map/depth_debug_scene.vert.spv
glslc shadow_map/depth_debug_scene.frag -o shadow_map/depth_debug_scene.frag.spv