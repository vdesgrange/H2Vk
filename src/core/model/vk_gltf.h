#pragma once

#include "tiny_gltf.h"
#include "vk_model.h"

class Model;

class ModelGLTF: public Model {
public:
    using Model::Model;

    bool load_model(const Device& device, const UploadContext& ctx, const char *filename) override;

protected:
    void load_images(const Device& device, const UploadContext& ctx, tinygltf::Model& input);
    void load_textures(tinygltf::Model& input);
    void load_materials(tinygltf::Model& input);
    void load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
    void load_scene(tinygltf::Model& input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
};
