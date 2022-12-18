#pragma once

#include "tiny_gltf.h"
#include "vk_mesh.h"

class Model;

class ModelGLTF: public Model {
public:
    bool load_model(VulkanEngine& engine, const char *filename) override;
    void print_type() override;

protected:
    void load_images(VulkanEngine& _engine, tinygltf::Model& input);
    void load_textures(tinygltf::Model& input);
    void load_materials(tinygltf::Model& input);
    void load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
    void load_scene(tinygltf::Model& input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
};
