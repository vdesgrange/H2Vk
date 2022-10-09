#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/hash.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <unordered_map>

#include "tiny_obj_loader.h"
#include "tiny_gltf.h"
#include "core/vk_types.h"

class Device;

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
    glm::vec3 color;
    glm::vec2 uv;

    bool operator==(const Vertex& other) const {
        return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
    }

    static VertexInputDescription get_vertex_description();
};

struct Node;

struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t materialIndex;
};

class Mesh {
public:
    std::vector<Vertex> _vertices;
    AllocatedBuffer _vertexBuffer;
    std::vector<Primitive> primitives;

    static Mesh cube();
    bool load_from_obj(const char* filename);
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
};

struct Image {
    VkImage _image;
    VmaAllocation _allocation;
    VkDescriptorSet _descriptorSet;
};

struct Textures {
    int32_t imageIndex;
};


class Model final {
public:
    std::vector<Image> images;
    std::vector<Textures> textures;
    std::vector<Materials> materials;
    std::vector<Node*> nodes;

    AllocatedBuffer vertices;

    Model(Device& device) : _device(device) {};
    ~Model();

    bool load_from_gltf(const char *filename);
private:
    const class Device& _device;

    void load_image(tinygltf::Model& input);
    void load_texture(tinygltf::Model& input);
    void load_material(tinygltf::Model& input);
    void load_scene(tinygltf::Model& input);
    void load_node(const tinygltf::Node& node, tinygltf::Model &input);
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^(hash<glm::vec3>()(vertex.normal) << 1);
        }
    };
}
