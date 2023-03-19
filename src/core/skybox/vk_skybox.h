#pragma once

#include <memory>

#include "core/vk_texture.h"
#include "core/vk_shaders.h"

class Model;
class Device;
class Texture;
class MeshManager;
class MaterialManager;
class PipelineBuilder;
struct UploadContext;

class Skybox final {
public:
    enum Type { box, sphere };
    std::shared_ptr<Model> _model;
    std::shared_ptr<Material> _material;
    Texture _background; // aka. background
    Texture _environment; // aka. radiance map
    Texture _prefilter; // aka. reflection map
    Texture _brdf; // BRDF

    Type _type = Type::box;
    bool _display = true;

    Skybox(Device& device, MeshManager& meshManager, UploadContext& uploadContext);
    ~Skybox();

    void load();
    void load_sphere_texture(const char* file, Texture& texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
    void setup_pipeline(MaterialManager& materialManager, std::vector<VkDescriptorSetLayout> setLayouts);
    void draw(VkCommandBuffer& commandBuffer);
    void build_command_buffer(VkCommandBuffer& commandBuffer, VkDescriptorSet* descriptor);
    void destroy();

private:
    class Device& _device;
    class UploadContext& _uploadContext;
    class MeshManager& _meshManager;

    void submit_texture();
};