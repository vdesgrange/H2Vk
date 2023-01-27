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
