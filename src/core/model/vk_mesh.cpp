#include "vk_mesh.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_command_buffer.h"
#include "core/vk_command_pool.h"
#include "core/vk_engine.h"
#include "core/vk_camera.h"


Model::Model(Device* device) : _device(device) {}

Model::~Model() {
    // this->destroy(); // break for some reason
}

Model::Model(const Model& rhs) : _device(rhs._device) {
    this->_images = rhs._images;
    this->_textures = rhs._textures;
    this->_materials = rhs._materials;

    for (auto it : rhs._nodes) {
        Node* node = new Node(*it);
        this->_nodes.push_back(node);
    }

    this->_indexesBuffer = rhs._indexesBuffer;
    this->_verticesBuffer = rhs._verticesBuffer;

    this->_indexBuffer = rhs._indexBuffer;
    this->_vertexBuffer = rhs._vertexBuffer;
}

Model& Model::operator=(const Model& rhs) {
    this->_images = rhs._images;
    this->_textures = rhs._textures;
    this->_materials = rhs._materials;

    for (auto it : rhs._nodes) {
        this->_nodes.push_back(std::move(it));
    }

    this->_indexesBuffer = rhs._indexesBuffer;
    this->_verticesBuffer = rhs._verticesBuffer;

    this->_indexBuffer = rhs._indexBuffer;
    this->_vertexBuffer = rhs._vertexBuffer;

    this->_device = rhs._device;

    return *this;
}

void Model::destroy() {
    for (auto node : _nodes) {
        delete node;
    }
    _nodes.clear();

    for (Image image : _images) {
        image._texture.destroy(*_device);
    }
    _images.clear();
    _materials.clear();
    _textures.clear();

    vmaDestroyBuffer(_device->_allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
    vmaDestroyBuffer(_device->_allocator, _indexBuffer.allocation._buffer, _indexBuffer.allocation._allocation);
}

VkDescriptorImageInfo Model::get_texture_descriptor(const size_t index)
{
    return _images[index]._texture._descriptor;
}


void Model::draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix = node->matrix;
        Node* parent = node->parent;
        while (parent) {
            nodeMatrix = parent->matrix * nodeMatrix;
            parent = parent->parent;
        }

        for (Primitive& primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                if (!_textures.empty() && primitive.materialIndex != -1) { // handle non-gltf meshes
                    Materials material = _materials[primitive.materialIndex];
                    // Textures texture = _textures[_materials[primitive.materialIndex].baseColorTextureIndex];  // Get the texture index for this primitive

                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &_images[material.baseColorTextureIndex]._descriptorSet, 0, nullptr);
                    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &_images[texture.imageIndex]._descriptorSet, 0, nullptr);
                }
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, instance);
            }
        }
    }

    for (auto& child : node->children) {
        draw_node(child, commandBuffer, pipelineLayout, instance);
    }
}

void Model::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind) {
    if (bind) {
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.allocation._buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    for (auto& node : _nodes) {
        draw_node(node, commandBuffer, pipelineLayout, instance);
    }
}

void Model::draw_obj(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance, bool bind) {
    if (bind) {
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.allocation._buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_indexesBuffer.size()), 1, 0, 0, instance);
}

//Mesh Mesh::cube() {
//    Mesh mesh{};
//
//    const std::array<std::array<int, 7>, 6> cube_faces = {{
//    {0, 4, 2, 6, -1, 0, 0}, // -x
//    {1, 3, 5, 7, +1, 0, 0}, // +x
//    {0, 1, 4, 5, 0, -1, 0}, // -y
//    {2, 6, 3, 7, 0, +1, 0}, // +y
//    {0, 2, 1, 3, 0, 0, -1}, // -z
//    {4, 5, 6, 7, 0, 0, +1} // +z
//    }};
//
//    for (int i = 0; i < cube_faces.size(); i++) {
//        std::array<int, 7> face = cube_faces[i];
//        for (int j = 0; j < 4; j++) {
//            int d = face[j];
//
//            Vertex vertex{};
//            vertex.position = {(d & 1) * 2 - 1, (d & 2) - 1, (d & 4) / 2 - 1};
//            vertex.normal = {face[4], face[5], face[6]};
//            vertex.uv = {j & 1, (j & 2) / 2};
//            vertex.color = {0, 255, 0};
//
//            mesh._vertices.push_back(vertex);
//        }
//    }
//
//    return mesh;
//}

VertexInputDescription Vertex::get_vertex_description()
{
    VertexInputDescription description;

    //we will have just 1 vertex buffer binding, with a per-vertex rate
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding = 0;
    mainBinding.stride = sizeof(Vertex);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    description.bindings.push_back(mainBinding);

    //Position will be stored at Location 0
    VkVertexInputAttributeDescription positionAttribute = {};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, position);

    //Normal will be stored at Location 1
    VkVertexInputAttributeDescription normalAttribute = {};
    normalAttribute.binding = 0;
    normalAttribute.location = 1;
    normalAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normalAttribute.offset = offsetof(Vertex, normal);

    //UV will be stored at Location 2
    VkVertexInputAttributeDescription  uvAttribute = {};
    uvAttribute.binding = 0;
    uvAttribute.location = 2;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(Vertex, uv);

    //Color will be stored at Location 3
    VkVertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 3;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(uvAttribute);
    description.attributes.push_back(colorAttribute);
    return description;
}