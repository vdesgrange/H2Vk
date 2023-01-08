#pragma once

#include "vk_gltf.h"

class ModelGLTF;

class ModelGLB final: public ModelGLTF {
public:
    using ModelGLTF::ModelGLTF;

    bool load_model(VulkanEngine& engine, const char *filename) override;
};

