#!/bin/bash

glslc shader_base.vert -o shader_base.vert.spv
glslc shader_base.frag -o shader_base.frag.spv
glslc red_shader_base.vert -o red_shader_base.vert.spv
glslc red_shader_base.frag -o red_shader_base.frag.spv
glslc tri_mesh.vert -o tri_mesh.vert.spv
