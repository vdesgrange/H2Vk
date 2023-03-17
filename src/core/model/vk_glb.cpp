#include "vk_glb.h"

bool ModelGLB::load_model(const Device& device, const UploadContext& ctx, const char *filename) {
    tinygltf::Model input;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (!loader.LoadBinaryFromFile(&input, &err, &warn, filename)) {
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