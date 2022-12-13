#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "vk_mesh.h"
#include "core/vk_device.h"
#include "core/vk_buffer.h"
#include "core/vk_initializers.h"
#include "core/vk_helpers.h"
#include "core/vk_command_buffer.h"
#include "core/vk_command_pool.h"
#include "core/vk_fence.h"
#include "core/vk_engine.h"
#include "core/vk_texture.h"
#include "core/vk_descriptor_builder.h"
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

void Model::load_image(VulkanEngine& engine, tinygltf::Model &input) {
    _images.resize(input.images.size());

    for (uint32_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image& gltfImage = input.images[i];
        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;

        if (gltfImage.component == 3) {
            bufferSize = gltfImage.width * gltfImage.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb = &gltfImage.image[0];
            for (uint32_t i = 0; i < gltfImage.width * gltfImage.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        } else {
            buffer = gltfImage.image.data(); // ou &gltfImage.image[0]
            bufferSize = gltfImage.image.size();
        }

        // _images[i]._textures.fromBuffer(buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height, vulkanDevice, copyQueue);
        vkutil::load_image_from_buffer(engine, buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM,   gltfImage.width,  gltfImage.height, _images[i]);

        if (deleteBuffer) {
            delete[] buffer;
        }
    }
}

void Model::load_texture(tinygltf::Model &input) {
    _textures.resize(input.textures.size());
    for (uint32_t i = 0; i < input.textures.size(); i++) {
        _textures[i].imageIndex = input.textures[i].source;
    }
}

void Model::load_material(tinygltf::Model &input) {
    _materials.resize(input.materials.size());
    for (uint32_t i = 0; i < input.materials.size(); i++) {
        tinygltf::Material gltfMaterial = input.materials[i];
        if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
            _materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            _materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
        }
    }
}

void Model::load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    Node* node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    if (iNode.matrix.size() == 16) {
        node->matrix = glm::make_mat4x4(iNode.matrix.data());
    } else {
        if (iNode.translation.size() == 3) {
            node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(iNode.translation.data())));
        }

        if (iNode.rotation.size() == 4) {
            glm::quat q = glm::make_quat(iNode.rotation.data());
            node->matrix *= glm::mat4(q);
        }

        if (iNode.scale.size() == 3) {
            node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(iNode.scale.data())));
        }
    }

    for (auto const& idx : iNode.children) {
        this->load_node(input.nodes[idx], input, node, indexBuffer, vertexBuffer);
    }

    if (iNode.mesh > -1) {
        const tinygltf::Mesh mesh = input.meshes[iNode.mesh];
        for (uint32_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& gltfPrimitive = mesh.primitives[i];
            uint32_t indexCount = 0;
            auto firstIndex = static_cast<uint32_t>(indexBuffer.size());
            auto vertexStart = static_cast<uint32_t>(vertexBuffer.size());

            const float* positionBuffer = nullptr;
            const float* normalsBuffer = nullptr;
            const float* texCoordsBuffer = nullptr;
            size_t vertexCount = 0;

            // Buffers, buffer views & accessors
            if (gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                vertexCount = accessor.count;
            }

            if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            for (size_t v = 0; v < vertexCount; v++) {
                Vertex vert{};
                vert.position = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
                vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
                vert.color = glm::vec3(1.0f);
                vertexBuffer.push_back(vert);
            }

            const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.indices];
            const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

            indexCount += static_cast<uint32_t>(accessor.count);

            // glTF supports different component types of indices
            switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    auto buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    auto buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    auto buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                break;
                }
                default:
                    std::cerr << "Index component type " << accessor.componentType << " not supported" << std::endl;
                return;
            }

            Primitive primitive{};
            primitive.firstIndex = firstIndex;
            primitive.indexCount = indexCount;
            primitive.materialIndex = gltfPrimitive.material;
            node->mesh.primitives.push_back(primitive);
        }
    }

    if (parent) {
        parent->children.push_back(node);
    } else {
        _nodes.push_back(node);
    }

}

void Model::load_node(tinyobj::attrib_t attrib, std::vector<tinyobj::shape_t>& shapes) {
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

void Model::load_scene(tinygltf::Model &input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    const tinygltf::Scene& scene = input.scenes[0];
    for (uint32_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = input.nodes[scene.nodes[i]];
        this->load_node(node, input, nullptr, indexBuffer, vertexBuffer);
    }
}

void Model::draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t instance) {
    if (!node->mesh.primitives.empty()) {
        glm::mat4 nodeMatrix = node->matrix;
        Node* parent = node->parent;
        while (parent) {
            nodeMatrix = parent->matrix * nodeMatrix;
            parent = parent->parent;
        }
        vkCmdPushConstants(commandBuffer, pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(glm::mat4),&nodeMatrix);
        // vkCmdPushConstants(commandBuffer,pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(MeshPushConstants),&constants);

        for (Primitive& primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
//                if (!_textures.empty() && primitive.materialIndex != -1) {
//                    // Get the texture index for this primitive
//                    Textures texture = _textures[_materials[primitive.materialIndex].baseColorTextureIndex];
//                    // Bind the descriptor for the current primitive's texture
//                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_images[texture.imageIndex]._descriptorSet, 0, nullptr);
//                }
                Textures texture = _textures[_materials[primitive.materialIndex].baseColorTextureIndex];
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_images[texture.imageIndex]._descriptorSet, 0, nullptr);
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

//void Model::descriptors(VulkanEngine& engine) {
//    std::vector<VkDescriptorPoolSize> sizes = {
//            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
//            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(this->_images.size()) }
//    };
//
////    _camBuffer = Buffer::create_buffer(*engine._device, sizeof(GPUCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
////
////    VkDescriptorBufferInfo camBInfo{};
////    camBInfo.buffer = _camBuffer._buffer; // frames[i]
////    camBInfo.offset = 0;
////    camBInfo.range = sizeof(GPUCameraData);
////
////    DescriptorBuilder::begin(*engine._layoutCache, *engine._allocator)
////        .bind_buffer(camBInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0)
////        .build(_descriptorSet, _descriptorSetLayouts.matrices, sizes);
//
//    for (auto& image : this->_images) {
//        VkDescriptorImageInfo imageBInfo;
//        imageBInfo.sampler = image._sampler;
//        imageBInfo.imageView = image._imageView;
//        imageBInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//
//        DescriptorBuilder::begin(*engine._layoutCache, *engine._allocator)
//            .bind_image(imageBInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0)
//            .build(image._descriptorSet, engine._descriptorSetLayouts.textures, sizes);
//    }
//}

bool Model::load_from_glb(VulkanEngine& engine, const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (!loader.LoadBinaryFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_image(engine, input);
    this->load_texture(input);
    this->load_material(input);
    this->load_scene(input, _indexesBuffer, _verticesBuffer);

    return true;
}

bool Model::load_from_gltf(VulkanEngine& engine, const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
//    std::vector<uint32_t> indexBuffer;
//    std::vector<Vertex> vertexBuffer;

    if (!loader.LoadASCIIFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_image(engine, input);
    this->load_texture(input);
    this->load_material(input);
    this->load_scene(input, _indexesBuffer, _verticesBuffer);

//    size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
//    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
//    this->_indexBuffer.count = static_cast<uint32_t>(indexBuffer.size());
//
//    AllocatedBuffer vertexStaging = Buffer::create_buffer(engine._device, vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
//    void* data;
//    vmaMapMemory(_engine._device->_allocator, vertexStaging._allocation, &data);
//    memcpy(data, vertexBuffer.data(), static_cast<size_t>(vertexBufferSize)); // number of vertex
//    vmaUnmapMemory(_engine._device->_allocator, vertexStaging._allocation);
//    _vertexBuffer = Buffer::create_buffer(*_engine._device, vertexBufferSize,  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
//    immediate_submit([=](VkCommandBuffer cmd) {
//        VkBufferCopy copy;
//        copy.dstOffset = 0;
//        copy.srcOffset = 0;
//        copy.size = vertexBufferSize;
//        vkCmdCopyBuffer(cmd, vertexStaging._buffer, _vertexBuffer._buffer, 1, &copy);
//    });
//
//    AllocatedBuffer indexStaging = Buffer::create_buffer(*_engine._device, indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
//    void* data2;
//    vmaMapMemory(engine._device->_allocator, indexStaging._allocation, &data);
//    memcpy(data2, indexBuffer.data(), static_cast<size_t>(indexBufferSize));
//    vmaUnmapMemory(engine._device->_allocator, indexStaging._allocation);
//    _indexBuffer.allocation = Buffer::create_buffer(*_engine._device, indexBufferSize,   VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
//    immediate_submit([=](VkCommandBuffer cmd) {
//        VkBufferCopy copy;
//        copy.dstOffset = 0;
//        copy.srcOffset = 0;
//        copy.size = indexBufferSize;
//        vkCmdCopyBuffer(cmd, indexStaging._buffer, _indexBuffer.allocation._buffer, 1, &copy);
//    });
//
//    vmaDestroyBuffer(engine._device->_allocator, vertexStaging._buffer, vertexStaging._allocation);
//    vmaDestroyBuffer(engine._device->_allocator, indexStaging._buffer, indexStaging._allocation);

    return true;
}

bool Model::load_from_obj(const char *filename) {
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

bool Mesh::load_from_obj(const char *filename) {
    std::unordered_map<Vertex, uint32_t> unique_vertices{};
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2],
            };

            vertex.color = vertex.normal;

            vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0], // ux
                    1 - attrib.texcoords[2 * index.texcoord_index + 1], // uy, 1 - uy because of vulkan coords.
            };

            _vertices.push_back(vertex);

        }
    }

    return true;
}

Mesh Mesh::cube() {
    Mesh mesh{};

    const std::array<std::array<int, 7>, 6> cube_faces = {{
    {0, 4, 2, 6, -1, 0, 0}, // -x
    {1, 3, 5, 7, +1, 0, 0}, // +x
    {0, 1, 4, 5, 0, -1, 0}, // -y
    {2, 6, 3, 7, 0, +1, 0}, // +y
    {0, 2, 1, 3, 0, 0, -1}, // -z
    {4, 5, 6, 7, 0, 0, +1} // +z
    }};

    for (int i = 0; i < cube_faces.size(); i++) {
        std::array<int, 7> face = cube_faces[i];
        for (int j = 0; j < 4; j++) {
            int d = face[j];

            Vertex vertex{};
            vertex.position = {(d & 1) * 2 - 1, (d & 2) - 1, (d & 4) / 2 - 1};
            vertex.normal = {face[4], face[5], face[6]};
            vertex.uv = {j & 1, (j & 2) / 2};
            vertex.color = {0, 255, 0};

            mesh._vertices.push_back(vertex);
        }
    }

    return mesh;
}

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