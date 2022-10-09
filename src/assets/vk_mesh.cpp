#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "vk_mesh.h"
#include "core/vk_device.h"

Model::~Model() {
    for (auto node : nodes) {
        delete node;
    }

    vmaDestroyBuffer(_device._allocator, vertices._buffer, vertices._allocation);
}

void Model::load_image(tinygltf::Model &input) {
    images.resize(input.images.size());
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
        // todo
        if (deleteBuffer) {
            delete[] buffer;
        }
    }
}

void Model::load_texture(tinygltf::Model &input) {
    textures.resize(input.textures.size());
    for (size_t i = 0; i < input.textures.size(); i++) {
        textures[i].imageIndex = input.textures[i].source;
    }
}

void Model::load_material(tinygltf::Model &input) {
    materials.resize(input.materials.size());
    for (size_t i = 0; i < input.materials.size(); i++) {
        tinygltf::Material gltfMaterial = input.materials[i];
        if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end()) {
            materials[i].baseColorFactor = glm::make_vec4(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
        }

        if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end()) {
            materials[i].baseColorTextureIndex = gltfMaterial.values["baseColorTexture"].TextureIndex();
        }
    }
}

void Model::load_scene(tinygltf::Model &input) {
    const tinygltf::Scene& scene = input.scenes[0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = input.nodes[scene.nodes[i]];
        this->load_node(node, input);
    }
}

void Model::load_node(const tinygltf::Node& node, tinygltf::Model& input) {

}

bool Model::load_from_gltf(const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;


    if (!loader.LoadASCIIFromFile(&input, &err, &warn, filename)) {
        std::cerr << warn << std::endl;
        std::cerr << err << std::endl;
        return false;
    }

    this->load_image(input);
    this->load_texture(input);
    this->load_material(input);
    this->load_scene(input);

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

