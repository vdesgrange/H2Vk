/*
*  H2Vk - GLTF file loader (updated version)
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

//#define TINYGLTF_IMPLEMENTATION // Might have duplicate error if constant and library are both defined and included. Comment to fix
//#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION // #define TINYGLTF_NO_STB_IMAGE_WRITE

#include "vk_gltf2.h"
#include "core/vk_buffer.h"
#include "core/vk_texture.h"
#include "core/utilities/vk_initializers.h"

bool ModelGLTF2::load_model(const Device& device, const UploadContext& ctx, const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_texture_samplers(input);
    this->load_textures(device, ctx, input);
    this->load_materials(input);
    this->load_scene(input, _indexesBuffer, _verticesBuffer);

    return true;
}

void ModelGLTF2::load_texture_samplers(tinygltf::Model& input) {
    for (tinygltf::Sampler smp : input.samplers) {
        Sampler sampler{};
        sampler.minFilter = getVkFilterMode(smp.minFilter);
        sampler.magFilter = getVkFilterMode(smp.magFilter);
        sampler.addressModeV = getVkSamplerMode(smp.wrapS);
        sampler.addressModeV = getVkSamplerMode(smp.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        _samplers.push_back(sampler);
    }
}

void ModelGLTF2::load_textures(const Device& device, const UploadContext& ctx, tinygltf::Model& input) {
    _images.reserve(input.textures.size() + 1);

    for (tinygltf::Texture tex : input.textures) {
        tinygltf::Image& gltfImage = input.images[tex.source];
        Sampler sampler;

        if (tex.sampler == -1) {
            sampler.minFilter = VK_FILTER_LINEAR;
            sampler.magFilter = VK_FILTER_LINEAR;
            sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        } else {
            sampler = _samplers[tex.sampler];
        }

        // Load image
        unsigned char* buffer = nullptr;
        VkDeviceSize bufferSize = 0;
        bool deleteBuffer = false;

        if (gltfImage.component == 3) { // RGB need conversion to RGBA
            bufferSize = gltfImage.width * gltfImage.height * 4;
            buffer = new unsigned char[bufferSize];
            unsigned char* rgba = buffer;
            unsigned char* rgb = &gltfImage.image[0];
            for (uint32_t i = 0; i < gltfImage.width * gltfImage.height; ++i) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4; // move of 4 chars
                rgb += 3; // move of 3 chars
            }
            deleteBuffer = true;
        } else {
            buffer = gltfImage.image.data(); // ou &gltfImage.image[0]
            bufferSize = gltfImage.image.size();
        }

        Image image;
        //        load_image(device, ctx, input, sampler, tex.source, image);
        image._texture.load_image_from_buffer(device, ctx, buffer, bufferSize, sampler, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height);
        image._texture._name = gltfImage.name.empty() ? "Unknown" : gltfImage.name;
        image._texture._uri = gltfImage.uri.empty() ? "Unknown" : gltfImage.uri;
        _images.push_back(image);

//        Image image;
//        image._texture._name = gltfImage.name.empty() ? "Unknown" : gltfImage.name;
//        image._texture._uri = gltfImage.uri.empty() ? "Unknown" : gltfImage.uri;

    }

    // === Empty texture ===
    unsigned char pixels[] = {0, 0, 0, 0};
    Image image;
    image._texture.load_image_from_buffer(device, ctx, pixels, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1);
    image._texture._name = "Empty";
    _images.back()._texture._uri = "Unknown";

}

void ModelGLTF2::load_materials(tinygltf::Model &input) {
    _materials.resize(input.materials.size());

    for (uint32_t i = 0; i < input.materials.size(); i++) {
        tinygltf::Material gltfMaterial = input.materials[i];

        _materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor.data());

        int baseColorTextureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
        _materials[i].baseColorTextureIndex = baseColorTextureIndex;
        // _materials[i].texCoordSets.baseColor = gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord;
        if (baseColorTextureIndex != -1) {
            _materials[i].baseColorTexture =  &this->_images[baseColorTextureIndex];
        } else {
            _materials[i].baseColorTexture = &this->_images.back();
        }

        int metallicRoughnessTextureIndex =  gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
        _materials[i].metallicRoughnessTextureIndex = metallicRoughnessTextureIndex;
        // _materials[i].texCoordSets.metallicRoughness = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;
        if (metallicRoughnessTextureIndex != -1) {
            _materials[i].metallicRoughnessTexture = &this->_images[metallicRoughnessTextureIndex];
        } else {
            _materials[i].metallicRoughnessTexture =  &this->_images.back();
        }

        int normalTextureIndex = gltfMaterial.normalTexture.index;
        _materials[i].normalTextureIndex = normalTextureIndex;
        // _materials[i].texCoordSets.normal = gltfMaterial.normalTexture.texCoord;
        if (normalTextureIndex != -1) {
            _materials[i].normalTexture = &this->_images[normalTextureIndex];
        } else {
            _materials[i].normalTexture =  &this->_images.back();
        }

        int aoTextureIndex = gltfMaterial.occlusionTexture.index;
        _materials[i].aoTextureIndex = aoTextureIndex;

        if (aoTextureIndex != -1) {
            _materials[i].aoTexture = &this->_images[aoTextureIndex];
        } else {
            _materials[i].aoTexture =  &this->_images.back();
        }

        int emissiveTextureIndex =  gltfMaterial.emissiveTexture.index;
        _materials[i].emissiveTextureIndex = emissiveTextureIndex;
        // _materials[i].texCoordSets.emissive = gltfMaterial.emissiveTexture.texCoord;
        if (emissiveTextureIndex != -1) {
            _materials[i].emissiveTexture =  &this->_images[emissiveTextureIndex];
        } else {
            _materials[i].emissiveTexture =  &this->_images.back();
        }
    }
}

void ModelGLTF2::load_image(const Device& device, const UploadContext& ctx, tinygltf::Model& input, Sampler& sampler, uint32_t source, Image& image) {
    tinygltf::Image& gltfImage = input.images[source];

    unsigned char* buffer = nullptr;
    VkDeviceSize bufferSize = 0;
    bool deleteBuffer = false;

    if (gltfImage.component == 3) { // RGB need conversion to RGBA
        bufferSize = gltfImage.width * gltfImage.height * 4;
        buffer = new unsigned char[bufferSize];
        unsigned char* rgba = buffer;
        unsigned char* rgb = &gltfImage.image[0];
        for (uint32_t i = 0; i < gltfImage.width * gltfImage.height; ++i) {
            memcpy(rgba, rgb, sizeof(unsigned char) * 3);
            rgba += 4; // move of 4 chars
            rgb += 3; // move of 3 chars
        }
        deleteBuffer = true;
    } else {
        buffer = gltfImage.image.data(); // ou &gltfImage.image[0]
        bufferSize = gltfImage.image.size();
    }

    image._texture.load_image_from_buffer(device, ctx, buffer, bufferSize, sampler, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height);
    image._texture._name = gltfImage.name.empty() ? "Unknown" : gltfImage.name;
    image._texture._uri = gltfImage.uri.empty() ? "Unknown" : gltfImage.uri;

    if (deleteBuffer) {
        delete[] buffer;
    }
}

void ModelGLTF2::load_scene(tinygltf::Model &input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    const tinygltf::Scene& scene = input.scenes[input.defaultScene > -1 ? input.defaultScene : 0];
    this->_name = scene.name.empty() ? "Unknown" : scene.name;

    for (uint32_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = input.nodes[scene.nodes[i]];
        this->load_node(node, input, nullptr, indexBuffer, vertexBuffer);
    }
}

void ModelGLTF2::load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    Node* node = new Node{};
    node->matrix = glm::mat4(1.f);
    node->parent = nullptr;

    if (!iNode.name.empty()) {
        node->name = iNode.name;
    }

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

        if (!mesh.name.empty()) {
            node->mesh.name = mesh.name;
        }

        for (uint32_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& gltfPrimitive = mesh.primitives[i];
            uint32_t indexCount = 0;
            auto firstIndex = static_cast<uint32_t>(indexBuffer.size());
            auto vertexStart = static_cast<uint32_t>(vertexBuffer.size());

            const float* positionBuffer = nullptr;
            const float* normalsBuffer = nullptr;
            const float* texCoordsBuffer = nullptr;
            const float* texCoordsBufferSet1 = nullptr;
            const float* colorsBuffer = nullptr;
            const float* tangentsBuffer = nullptr;
            uint32_t numColorComponents;
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

            if (gltfPrimitive.attributes.find("COLOR_0") != gltfPrimitive.attributes.end()) {
                const tinygltf::Accessor& colorAccessor = input.accessors[gltfPrimitive.attributes.find("COLOR_0")->second];
                const tinygltf::BufferView& colorView = input.bufferViews[colorAccessor.bufferView];
                numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
                colorsBuffer = reinterpret_cast<const float*>(&(input.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
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
                if (colorsBuffer) {
                    switch (numColorComponents) {
                        case 3:
                            vert.color = glm::vec4(glm::make_vec3(&colorsBuffer[v * 3]), 1.0f);
                        case 4:
                            vert.color = glm::make_vec4(&colorsBuffer[v * 4]);
                        default:
                            vert.color = glm::vec4(0.f);
                    }
                }
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

VkFilter ModelGLTF2::getVkFilterMode(int32_t filterMode) {
    switch (filterMode) {
        case -1:
        case 9728:
        case 9984:
        case 9985:
            return VK_FILTER_NEAREST;
        case 9729:
        case 9986:
        case 9987:
            return VK_FILTER_LINEAR;
    }

    return VK_FILTER_NEAREST;
}

VkSamplerAddressMode ModelGLTF2::getVkSamplerMode(int32_t samplerMode) {
    switch (samplerMode) {
        case -1:
        case 10497:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case 33071:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 33648:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }

    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}