#pragma once

#include "vk_gltf.h"

class ModelGLTF;

class ModelGLB final: public ModelGLTF {
public:
    bool load_model(VulkanEngine& engine, const char *filename) override;
    void print_type() override;
};

