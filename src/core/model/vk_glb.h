/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "vk_gltf.h"

class ModelGLTF;

class ModelGLB final: public ModelGLTF {
public:
    using ModelGLTF::ModelGLTF;

    bool load_model(const Device& device, const UploadContext& ctx, const char *filename) override;
};

