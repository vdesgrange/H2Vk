#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/hash.hpp"
#include <vector>
#include <iostream>
#include <atomic>

#include "core/manager/vk_system_manager.h"
#include "core/vk_texture.h"
#include "core/vk_buffer.h"
#include "core/utilities/vk_types.h"
#include "core/manager/vk_mesh_manager.h"
#include "core/vk_descriptor_builder.h"

class Device;
class DescriptorLayoutCache;
class DescriptorAllocator;

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
    glm::vec4 tangent;

    bool operator==(const Vertex& other) const {
        return position == other.position && color == other.color && uv == other.uv; // && normal == other.normal && tangent == other.tangent;
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

struct Mesh {
    std::string name = "Mesh";
    std::vector<Primitive> primitives;
};

struct Node {
    std::string name = "Node";
    Node* parent;  // Node*
    std::vector<Node*> children;  // Node*
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
    uint32_t metallicRoughnessTextureIndex;
    uint32_t aoTextureIndex;
    uint32_t emissiveTextureIndex;

    VkDescriptorSet _descriptorSet; // access texture from the fragment shader

    bool pbr = false;

    struct Properties {
        glm::vec4 albedo; // rgb + w for opacity
        float metallic;
        float roughness;
        float ao;
    } properties;
};
typedef Materials::Properties PBRProperties;

struct Image {
    Texture _texture;
    VkDescriptorSet _descriptorSet; // access texture from the fragment shader
};

struct Textures {
    int32_t imageIndex;
};

class Model : public Entity {
protected:
    static std::atomic<uint32_t> nextID;

public:
    uint32_t _uid {0};
    std::string _name = "Unnamed";
    std::vector<Image> _images {};
    std::vector<Textures> _textures {};
    std::vector<Materials> _materials {};
    std::vector<Node*> _nodes {};

    std::vector<uint32_t> _indexesBuffer {};
    std::vector<Vertex> _verticesBuffer {};

    struct {
        uint32_t count {}; // useless?
        AllocatedBuffer allocation {};
    } _indexBuffer {};
    AllocatedBuffer _vertexBuffer {};

    Model() = delete;
    Model(Device* device);
    Model(const Model& rhs);
    ~Model();
    Model& operator=(const Model& rhs);

    virtual bool load_model(const Device& device, const UploadContext& ctx, const char *filename) { return false; };

    void destroy();
    void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind);
    VkDescriptorImageInfo get_texture_descriptor(const size_t index);
    void setup_descriptors(DescriptorLayoutCache& layoutCache, DescriptorAllocator& allocator, VkDescriptorSetLayout& setLayout);

protected:
    void draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance);

private:
    Device* _device {nullptr};
};