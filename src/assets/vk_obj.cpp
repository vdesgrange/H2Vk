#define TINYOBJLOADER_IMPLEMENTATION // Might have duplicate error if constant and library are both defined and included. Comment to fix

#include <unordered_map>
#include "vk_obj.h"

void ModelOBJ::print_type() {
    std::cout << "je suis obj" << std::endl;
}

bool ModelOBJ::load_model(VulkanEngine& engine, const char *filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    std::string warn;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_node(attrib, shapes);
    return true;
}

void ModelOBJ::load_node(tinyobj::attrib_t attrib, std::vector<tinyobj::shape_t>& shapes) {
    Node* node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) { // equivalent to list all primitive and children
        auto firstIndex = static_cast<uint32_t>(_indexesBuffer.size());

        const tinyobj::mesh_t mesh = shape.mesh;
        for (const auto& index : mesh.indices) { // equivalent to loop over vertexCount
            Vertex vertex{};

            vertex.position = glm::vec3({
                                                attrib.vertices[3 * index.vertex_index + 0],
                                                attrib.vertices[3 * index.vertex_index + 1],
                                                attrib.vertices[3 * index.vertex_index + 2]
                                        });

            vertex.normal = glm::normalize(glm::vec3({
                                                             attrib.normals[3 * index.normal_index + 0],
                                                             attrib.normals[3 * index.normal_index + 1],
                                                             attrib.normals[3 * index.normal_index + 2]
                                                     }));

            vertex.color = vertex.normal;
//            vertex.color = glm::vec3({ // to get color represented by the normal
//                attrib.colors[3 * index.vertex_index + 0],
//                attrib.colors[3 * index.vertex_index + 1],
//                attrib.colors[3 * index.vertex_index + 2]
//            });

            vertex.uv = glm::make_vec2(glm::vec2({
                                                         attrib.texcoords[2 * index.texcoord_index + 0], // ux
                                                         1 - attrib.texcoords[2 * index.texcoord_index +1], // uy, 1 - uy because of vulkan coords
                                                 }));

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(_verticesBuffer.size());
                _verticesBuffer.push_back(vertex);
            }

            _indexesBuffer.push_back(uniqueVertices[vertex]); // push duplicate vertex or only new one ?
        }

        Primitive primitive{};
        primitive.firstIndex = firstIndex;
        primitive.indexCount = _indexesBuffer.size();
        primitive.materialIndex = -1; // not used here?
        node->mesh.primitives.push_back(primitive);
    }

    _nodes.push_back(node);

}

