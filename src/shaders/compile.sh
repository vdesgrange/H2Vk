#!/bin/bash

glslc scene.frag -o scene.frag.spv
glslc mesh.vert -o mesh.vert.spv
glslc mesh_tex.vert -o mesh_tex.vert.spv
glslc scene_tex.frag -o scene_tex.frag.spv
