#pragma once

#include "vk_gltf.h"

class ModelGLTF;

class ModelGLB final: public ModelGLTF {
public:
    using ModelGLTF::ModelGLTF;

    bool load_model(const Device& device, const UploadContext& ctx, const char *filename) override;
};

