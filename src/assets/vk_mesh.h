#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/hash.hpp"
#include <vector>
#include <iostream>

#include "core/vk_texture.h"
#include "core/vk_types.h"
#include "core/vk_mesh_manager.h"

class Device;
class VulkanEngine;

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 color;

    bool operator==(const Vertex& other) const {
        return position == other.position && color == other.color && uv == other.uv; // && normal == other.normal;
    }
    static VertexInputDescription get_vertex_description();
};

namespace std {
    template <> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^(hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

struct Node;

struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t materialIndex;
};

//class Mesh {
//public:
//    // std::vector<Vertex> _vertices;
//
//    std::vector<Primitive> primitives;
//
//    // AllocatedBuffer _vertexBuffer;
//    // static Mesh cube();
//    // bool load_from_obj(const char* filename);
//};

struct Mesh {
    std::vector<Primitive> primitives;
};

struct Node {
    Node* parent;
    std::vector<Node*> children;
    Mesh mesh;
    glm::mat4 matrix;
    ~Node() {
        for (auto& child : children) {
            delete child;
        }
    }
};

struct Materials {
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    uint32_t baseColorTextureIndex;
    uint32_t normalTextureIndex;
};

struct Image {
    Texture _texture;

//    VkImage _image;
//    VkImageLayout _imageLayout;
//    VmaAllocation _allocation;
//    VkImageView _imageView;
//    VkSampler _sampler;
//    VkDescriptorImageInfo _descriptor;

    VkDescriptorSet _descriptorSet; // access texture from the fragment shader

//    void updateDescriptor() {
//        _descriptor.sampler = _sampler;
//        _descriptor.imageView = _imageView;
//        _descriptor.imageLayout = _imageLayout;
//    }
};

struct Textures {
    int32_t imageIndex;
};

class Model {
public:
    std::vector<Image> _images;
    std::vector<Textures> _textures;
    std::vector<Materials> _materials;
    std::vector<Node*> _nodes;

    std::vector<uint32_t> _indexesBuffer;
    std::vector<Vertex> _verticesBuffer;

    struct {
        uint32_t count;
        AllocatedBuffer allocation;
    } _indexBuffer;
    AllocatedBuffer _vertexBuffer;
    AllocatedBuffer _camBuffer;

    virtual bool load_model(VulkanEngine& engine, const char *filename) { return false; };
    virtual void print_type() {};

    void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind);
    void draw_obj(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 transformMatrix, uint32_t instance, bool bind=false);
    void draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance);

protected:
    void immediate_submit(VulkanEngine& engine, std::function<void(VkCommandBuffer cmd)>&& function);
};