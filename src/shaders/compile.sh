#!/bin/bash

glslc triangle.vert -o triangle.vert.spv
glslc scene.frag -o scene.frag.spv
glslc mesh.vert -o mesh.vert.spv
glslc texture_lig.frag -o texture_lig.frag.spv
glslc model_mesh.vert -o model_mesh.vert.spv
glslc model_mesh.frag -o model_mesh.frag.spv
glslc mesh_tex.vert -o mesh_tex.vert.spv
glslc scene_tex.frag -o scene_tex.frag.spv
