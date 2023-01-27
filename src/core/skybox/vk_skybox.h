#pragma once

#include <memory>

#include "core/vk_texture.h"
#include "core/vk_shaders.h"

class Model;
class Device;
class Texture;
class TextureManager;
class MeshManager;
class PipelineBuilder;
struct UploadContext;

class Skybox final {
public:
    enum Type { box, sphere };
    std::shared_ptr<Model> _model;
    std::shared_ptr<Material> _material;
    Texture _texture;
    Type _type = Type::box;

    Skybox(Device& device, PipelineBuilder& pipelineBuilder, TextureManager& textureManager, MeshManager& meshManager, UploadContext& uploadContext);
    ~Skybox();

    void load();
    void load_cube_texture();
    void load_sphere_texture();
    void setup_descriptor();
    void setup_pipeline(PipelineBuilder& pipelineBuilder, std::vector<VkDescriptorSetLayout> setLayouts);
    void draw(VkCommandBuffer& commandBuffer);
    void destroy();

private:
    class Device& _device;
    class UploadContext& _uploadContext;
    class PipelineBuilder& _pipelineBuilder;
    class TextureManager& _textureManager;
    class MeshManager& _meshManager;
};