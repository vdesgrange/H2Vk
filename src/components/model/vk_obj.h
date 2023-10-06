/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "tiny_obj_loader.h"
#include "vk_model.h"

class Model;

class ModelOBJ final: public Model {
public:
    using Model::Model;

    bool load_model(const Device& device, const UploadContext& ctx, const char *filename) override;

private:
    void load_node(tinyobj::attrib_t attrib, std::vector<tinyobj::shape_t>& shapes);
    void load_images(const Device& device, const UploadContext& ctx);
};
