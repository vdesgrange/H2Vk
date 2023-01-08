#define TINYGLTF_IMPLEMENTATION // Might have duplicate error if constant and library are both defined and included. Comment to fix
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION // #define TINYGLTF_NO_STB_IMAGE_WRITE

#include "vk_gltf.h"

bool ModelGLTF::load_model(VulkanEngine& engine, const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_images(engine, input);
    this->load_textures(input);
    this->load_materials(input);
    this->load_scene(input, _indexesBuffer, _verticesBuffer);
    return true;
}

void ModelGLTF::load_images(VulkanEngine& engine, tinygltf::Model &input) {
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

        _images[i]._texture.load_image_from_buffer(engine, buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height);
        // vkutil::load_image_from_buffer(engine, buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM,   gltfImage.width,  gltfImage.height, _images[i]);

        if (deleteBuffer) {
            delete[] buffer;
        }
    }
}

void ModelGLTF::load_textures(tinygltf::Model &input) {
    _textures.resize(input.textures.size());
    for (uint32_t i = 0; i < input.textures.size(); i++) {
        _textures[i].imageIndex = input.textures[i].source;
    }
}

void ModelGLTF::load_materials(tinygltf::Model &input) {
    _materials.resize(input.materials.size());

    for (uint32_t i = 0; i < input.materials.size(); i++) {
        tinygltf::Material gltfMaterial = input.materials[i];
        if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
            _materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            _materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
        }

        if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end()) {
            _materials[i].normalTextureIndex = gltfMaterial.additionalValues["normalTexture"].TextureIndex();
        }
    }
}

void ModelGLTF::load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    Node* node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

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

    if (iNode.matrix.size() == 16) {
        node->matrix = glm::make_mat4x4(iNode.matrix.data());
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
            const float* tangentsBuffer = nullptr;
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

            if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& accessor = input.accessors[gltfPrimitive.attributes.find("TANGENT")->second];
                const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            for (size_t v = 0; v < vertexCount; v++) {
                Vertex vert{};
                vert.position = glm::vec3(glm::make_vec3(&positionBuffer[v * 3]));
                vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec2(0.0f); // glm::vec3(0.0f)
                vert.color = glm::vec3(0.f);
                vert.tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f);
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

void ModelGLTF::load_scene(tinygltf::Model &input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    const tinygltf::Scene& scene = input.scenes[0];
    for (uint32_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = input.nodes[scene.nodes[i]];
        this->load_node(node, input, nullptr, indexBuffer, vertexBuffer);
    }
}

//void ModelGLTF::descriptors(VulkanEngine& engine) {
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