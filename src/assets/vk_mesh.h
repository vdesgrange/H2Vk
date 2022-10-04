#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"
#include <vector>
#include <array>
#include <iostream>
#include <unordered_map>

#include "tiny_obj_loader.h"
#include "core/vk_types.h"

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

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^(hash<glm::vec3>()(vertex.normal) << 1);
        }
    };
}

class Mesh {
public:
    std::vector<Vertex> _vertices;
    AllocatedBuffer _vertexBuffer;

    static Mesh cube();
    bool load_from_obj(const char* filename);
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};
