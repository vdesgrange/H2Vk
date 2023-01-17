#pragma once

#include <memory>

#include "core/vk_texture.h"
#include "core/vk_shaders.h"

class Model;
class Device;
class Texture;
// class Material;
class TextureManager;
class MeshManager;
class PipelineBuilder;
struct UploadContext;

class Skybox final {
public:
    std::shared_ptr<Model> _cube;
    std::shared_ptr<Material> _material;
    Texture _texture;

    Skybox(Device& device, PipelineBuilder& pipelineBuilder, TextureManager& textureManager, MeshManager& meshManager, UploadContext& uploadContext);
    ~Skybox();

    void load();
    void load_texture();
    void destroy();

private:
    class Device& _device;
    class UploadContext& _uploadContext;
    class PipelineBuilder& _pipelineBuilder;
    class TextureManager& _textureManager;
    class MeshManager& _meshManager;
};