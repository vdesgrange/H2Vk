#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "vk_mesh.h"
#include "core/vk_device.h"

Model::~Model() {
    for (auto node : _nodes) {
        delete node;
    }

    vmaDestroyBuffer(_device._allocator, _vertices._buffer, _vertices._allocation);
}

void Model::load_image(tinygltf::Model &input) {
    _images.resize(input.images.size());

    for (size_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image& gltfImage = input.images[i];
        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;

        if (gltfImage.component == 3) {
            bufferSize = gltfImage.width * gltfImage.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb = &gltfImage.image[0];
            for (size_t i = 0; i < gltfImage.width * gltfImage.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuffer = true;
        } else {
            buffer = gltfImage.image.data(); // ou &gltfImage.image[0]
            bufferSize = gltfImage.image.size();
        }
        // todo : load texture from image buffer
        // _images[i]._textures.fromBuffer(buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height, vulkanDevice, copyQueue);

        if (deleteBuffer) {
            delete[] buffer;
        }
    }
}

void Model::load_texture(tinygltf::Model &input) {
    _textures.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        _textures[i].imageIndex = input.textures[i].source;
    }
}

void Model::load_material(tinygltf::Model &input) {
    _materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        tinygltf::Material gltfMaterial = input.materials[i];
        if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
            _materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            _materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
        }
    }
}

void Model::load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t> indexBuffer, std::vector<Vertex>& vertexBuffer) {
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
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& gltfPrimitive = mesh.primitives[i];
            uint32_t indexCount = 0;
            uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
            uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());

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
                    const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indexBuffer.push_back(buf[index] + vertexStart);
                    }
                break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
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

void Model::load_scene(tinygltf::Model &input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    const tinygltf::Scene& scene = input.scenes[0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = input.nodes[scene.nodes[i]];
        this->load_node(node, input, nullptr, indexBuffer, vertexBuffer);
    }
}

void Model::draw_node(Node* node, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout) {
    if (node->mesh.primitives.size() > 0) {
        glm::mat4 nodeMatrix = node->matrix;
        Node* parent = node->parent;
        while (parent) {
            nodeMatrix = parent->matrix * nodeMatrix;
            parent = parent->parent;
        }

        vkCmdPushConstants(nullptr, nullptr,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(glm::mat4),&nodeMatrix);
        for (Primitive& primitive : node->mesh.primitives) {
            if (primitive.indexCount > 0) {
                // Get the texture index for this primitive
                Textures texture = _textures[_materials[primitive.materialIndex].baseColorTextureIndex];
                // Bind the descriptor for the current primitive's texture

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &_images[texture.imageIndex]._descriptorSet, 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
            }
        }
    }

    for (auto& child : node->children) {
        draw_node(child, commandBuffer, pipelineLayout);
    }
}

void Model::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout) {
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer._buffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, _indexBuffer.allocation._buffer, 0, VK_INDEX_TYPE_UINT32);
    for (auto& node : _nodes) {
        draw_node(node, commandBuffer, pipelineLayout);
    }
}

bool Model::load_from_glb(const char *filename) {
    tinygltf::Model input;
    std::string err;
    std::string warn;
    tinygltf::TinyGLTF loader;

    if (!loader.LoadBinaryFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }
}

bool Model::load_from_gltf(const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    std::vector<uint32_t> indexBuffer;
    std::vector<Vertex> vertexBuffer;

    if (!loader.LoadASCIIFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_image(input);
    this->load_texture(input);
    this->load_material(input);
    this->load_scene(input, indexBuffer, vertexBuffer);

    size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
    size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
    this->_indexBuffer.count = static_cast<uint32_t>(indexBuffer.size());

//    struct StagingBuffer {
//        VkBuffer buffer;
//        VkDeviceMemory memory;
//    } vertexStaging, indexStaging;

    return true;
}

bool Model::load_from_obj(const char *filename) {
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

    std::vector<Vertex> vertexBuffer{};

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

            vertex.color = vertex.normal; // glm::vec3(1.0f);

            vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0], // ux
                    1 - attrib.texcoords[2 * index.texcoord_index + 1], // uy, 1 - uy because of vulkan coords.
            };

            vertexBuffer.push_back(vertex);

        }
    }

    size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);

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
            vertex.color = {0, 255, 0};
            vertex.uv = {j & 1, (j & 2) / 2};

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

    //Color will be stored at Location 2
    VkVertexInputAttributeDescription colorAttribute = {};
    colorAttribute.binding = 0;
    colorAttribute.location = 2;
    colorAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, color);

    //UV will be stored at Location 3
    VkVertexInputAttributeDescription  uvAttribute = {};
    uvAttribute.binding = 0;
    uvAttribute.location = 3;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(Vertex, uv);

    description.attributes.push_back(positionAttribute);
    description.attributes.push_back(normalAttribute);
    description.attributes.push_back(colorAttribute);
    description.attributes.push_back(uvAttribute);
    return description;
}

