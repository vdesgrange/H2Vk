/*
*  H2Vk - GLTF file loader
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "tiny_gltf.h"
#include "vk_model.h"

class Model;

class ModelGLTF2: public Model {
public:
    using Model::Model;

    bool load_model(const Device& device, const UploadContext& ctx, const char *filename) override;

protected:
    void load_texture_samplers(tinygltf::Model& input);
    void load_textures(const Device& device, const UploadContext& ctx, tinygltf::Model& input);
    void load_materials(tinygltf::Model& input);
    void load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
    void load_scene(tinygltf::Model& input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

private:
    VkFilter getVkFilterMode(int32_t filterMode);
    VkSamplerAddressMode getVkSamplerMode(int32_t samplerMode);
};

