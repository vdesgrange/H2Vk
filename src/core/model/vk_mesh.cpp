#include "vk_mesh.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_initializers.h"
#include "core/vk_helpers.h"
#include "core/vk_command_buffer.h"
#include "core/vk_command_pool.h"
#include "core/vk_engine.h"
#include "core/vk_camera.h"

//Model::~Model() {
//    for (auto node : _nodes) {
//        delete node;
//    }
//
//    for (Image image : _images) {
//        vkDestroyImageView(_engine._device->_logicalDevice, image._imageView, nullptr);
//        vmaDestroyImage(_engine._device->_allocator, image._image, image._allocation);  // destroyImage + vmaFreeMemory
//        vkDestroySampler(_engine._device->_logicalDevice, image._sampler, nullptr);
//    }
//
//    vmaDestroyBuffer(_engine._device->_allocator, _vertexBuffer._buffer, _vertexBuffer._allocation);
//    vmaDestroyBuffer(_engine._device->_allocator, _indexBuffer.allocation._buffer, _indexBuffer.allocation._allocation);
//}


void Model::draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix = node->matrix;
        Node* parent = node->parent;
        while (parent) {
            nodeMatrix = parent->matrix * nodeMatrix;
            parent = parent->parent;
        }

//        void* objectData;
//        vmaMapMemory(_device->_allocator, get_current_frame().objectBuffer._allocation, &objectData);
//        GPUObjectData* objectSSBO = (GPUObjectData*)objectData;
//        objectSSBO[instance].model = nodeMatrix;
//        vmaUnmapMemory(_device->_allocator, get_current_frame().objectBuffer._allocation);

         // vkCmdPushConstants(commandBuffer, pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(glm::mat4),&nodeMatrix);
        // vkCmdPush Constants(commandBuffer,pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(MeshPushConstants),&constants);

        for (Primitive& primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                if (!_textures.empty() && primitive.materialIndex != -1) { // handle non-gltf meshes
                    // Get the texture index for this primitive
                    Textures texture = _textures[_materials[primitive.materialIndex].baseColorTextureIndex];
                    // Bind the descriptor for the current primitive's texture
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &_images[texture.imageIndex]._descriptorSet, 0, nullptr);
                    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_images[texture.imageIndex]._descriptorSet, 0, nullptr);
                }
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, instance); // i
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

void Model::draw_obj(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, glm::mat4 transformMatrix, uint32_t instance, bool bind) {
    if (bind) {
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.allocation._buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    // MeshPushConstants constants;
    // constants.render_matrix = transformMatrix;  // expected in draw_node() and node->matrix
    // vkCmdPushConstants(commandBuffer,pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(MeshPushConstants),&constants);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_indexesBuffer.size()), 1, 0, 0, instance);
    // vkCmdDraw(commandBuffer, static_cast<uint32_t>(this->_verticesBuffer.size()), 1, 0, instance);
}



// bool Mesh::load_from_obj(const char *filename) {
//    std::unordered_map<Vertex, uint32_t> unique_vertices{};
//    tinyobj::attrib_t attrib;
//    std::vector<tinyobj::shape_t> shapes;
//    std::vector<tinyobj::material_t> materials;
//    std::string warn, err;
//
//    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr)) {
//        std::cerr << warn << std::endl;
//        std::cerr << err << std::endl;
//        return false;
//    }
//
//    for (const auto& shape : shapes) {
//        for (const auto& index : shape.mesh.indices) {
//            Vertex vertex{};
//
//            vertex.position = {
//                    attrib.vertices[3 * index.vertex_index + 0],
//                    attrib.vertices[3 * index.vertex_index + 1],
//                    attrib.vertices[3 * index.vertex_index + 2]
//            };
//
//            vertex.normal = {
//                    attrib.normals[3 * index.normal_index + 0],
//                    attrib.normals[3 * index.normal_index + 1],
//                    attrib.normals[3 * index.normal_index + 2],
//            };
//
//            vertex.color = vertex.normal;
//
//            vertex.uv = {
//                    attrib.texcoords[2 * index.texcoord_index + 0], // ux
//                    1 - attrib.texcoords[2 * index.texcoord_index + 1], // uy, 1 - uy because of vulkan coords.
//            };
//
//            _vertices.push_back(vertex);
//
//        }
//    }
//
//    return true;
//}

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

void Model::immediate_submit(VulkanEngine& engine, std::function<void(VkCommandBuffer cmd)>&& function) {  // ATTENTION : DUPLICA
    VkCommandBuffer cmd = engine._uploadContext._commandBuffer->_mainCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submitInfo = vkinit::submit_info(&cmd);
    VK_CHECK(vkQueueSubmit(engine._device->get_graphics_queue(), 1, &submitInfo, engine._uploadContext._uploadFence->_fence));

    vkWaitForFences(engine._device->_logicalDevice, 1, &engine._uploadContext._uploadFence->_fence, true, 9999999999);
    vkResetFences(engine._device->_logicalDevice, 1, &engine._uploadContext._uploadFence->_fence);

    vkResetCommandPool(engine._device->_logicalDevice, engine._uploadContext._commandPool->_commandPool, 0);
}