/*
*  H2Vk - GLTF file loader
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#define TINYGLTF_IMPLEMENTATION // Might have duplicate error if constant and library are both defined and included. Comment to fix
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION // #define TINYGLTF_NO_STB_IMAGE_WRITE

#include "vk_gltf.h"

bool ModelGLTF::load_model(const Device& device, const UploadContext& ctx, const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (!loader.LoadASCIIFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_images(device, ctx, input);
    this->load_textures(input);
    this->load_materials(input);
    this->load_scene(input, _indexesBuffer, _verticesBuffer);
    return true;
}

void ModelGLTF::load_images(const Device& device, const UploadContext& ctx, tinygltf::Model &input) {
    _images.resize(input.images.size() + 1); // Empty texture for mesh without material

    for (uint32_t i = 0; i < input.images.size(); i++) {
        tinygltf::Image& gltfImage = input.images[i];
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

        _images[i]._texture.load_image_from_buffer(device, ctx, buffer, bufferSize, VK_FORMAT_R8G8B8A8_UNORM, gltfImage.width, gltfImage.height);
        _images[i]._texture._name = gltfImage.name.empty() ? "Unknown" : gltfImage.name;
        _images[i]._texture._uri = gltfImage.uri.empty() ? "Unknown" : gltfImage.uri;

        if (deleteBuffer) {
            delete[] buffer;
        }

    }

    // === Empty texture ===
    unsigned char pixels[] = {0, 0, 0, 0};
    _images.back()._texture.load_image_from_buffer(device, ctx, pixels, 4, VK_FORMAT_R8G8B8A8_UNORM, 1, 1);
    _images.back()._texture._name = "Empty";
    _images.back()._texture._uri = "Unknown";
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

        if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end()) {
            _materials[i].metallicFactor = static_cast<float>(gltfMaterial.values["metallicFactor"].Factor());
        }

        if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end()) {
            _materials[i].roughnessFactor = static_cast<float>(gltfMaterial.values["roughnessFactor"].Factor());
        }

        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            uint32_t baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
            _materials[i].baseColorTextureIndex = baseColorTextureIndex;
            _materials[i].baseColorTexture =  &this->_images[_textures[baseColorTextureIndex].imageIndex]; // baseColorTextureIndex
        } else {
            _materials[i].baseColorTexture =  &this->_images.back();
        }

        if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end()) {
            uint32_t normalTextureIndex = gltfMaterial.additionalValues["normalTexture"].TextureIndex();
            _materials[i].normalTextureIndex = normalTextureIndex;
            _materials[i].normalTexture = &this->_images[_textures[normalTextureIndex].imageIndex]; // normalTextureIndex
        } else {
            _materials[i].normalTexture =  &this->_images.back();
        }


        if (gltfMaterial.additionalValues.find("metallicRoughnessTexture") != gltfMaterial.additionalValues.end()) {
            uint32_t metallicRoughnessTextureIndex = gltfMaterial.additionalValues["metallicRoughnessTexture"].TextureIndex();
            _materials[i].metallicRoughnessTextureIndex = metallicRoughnessTextureIndex;
            _materials[i].metallicRoughnessTexture = &this->_images[_textures[metallicRoughnessTextureIndex].imageIndex]; // metallicRoughnessTextureIndex
        } else {
            _materials[i].metallicRoughnessTexture =  &this->_images.back();
        }

        if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end()) {
            uint32_t aoTextureIndex = gltfMaterial.additionalValues["occlusionTexture"].TextureIndex();
            _materials[i].aoTextureIndex = aoTextureIndex;
            _materials[i].aoTexture = &this->_images[_textures[aoTextureIndex].imageIndex]; // aoTextureIndex
        } else {
            _materials[i].aoTexture =  &this->_images.back();
        }

        if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end()) {
            uint32_t emissiveTextureIndex = gltfMaterial.additionalValues["emissiveTexture"].TextureIndex();
            _materials[i].emissiveTextureIndex = emissiveTextureIndex;
            _materials[i].emissiveTexture = &this->_images[_textures[emissiveTextureIndex].imageIndex]; // emissiveTextureIndex
        } else {
            _materials[i].emissiveTexture =  &this->_images.back();
        }

    }
}

void ModelGLTF::load_node(const tinygltf::Node& iNode, tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
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
            uint32_t vertexCount = 0;

            auto firstIndex = static_cast<uint32_t>(indexBuffer.size());
            auto vertexStart = static_cast<uint32_t>(vertexBuffer.size());

            const float* positionBuffer = nullptr;
            const float* normalsBuffer = nullptr;
            const float* texCoordsBuffer = nullptr;
            const float* colorsBuffer = nullptr;
            const float* tangentsBuffer = nullptr;
            uint32_t numColorComponents;

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
        this->_nodes.push_back(node);
    }

}

void ModelGLTF::load_scene(tinygltf::Model &input, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer) {
    const tinygltf::Scene& scene = input.scenes[input.defaultScene > -1 ? input.defaultScene : 0];    // = input.scenes[0];
    this->_name = scene.name.empty() ? "Unknown" : scene.name;

    for (uint32_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = input.nodes[scene.nodes[i]];
        this->load_node(node, input, nullptr, indexBuffer, vertexBuffer);
    }
}
