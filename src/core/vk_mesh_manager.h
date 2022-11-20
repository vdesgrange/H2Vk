#pragma once

#include <string>
#include <unordered_map>

#include "VkBootstrap.h"


class Device;
class Mesh;
class Model;
class Fence;
class CommandPool;
class CommandBuffer;

struct UploadContext {
    Fence* _uploadFence;
    CommandPool* _commandPool;
    CommandBuffer* _commandBuffer;
};

class MeshManager final {
public:
    std::unordered_map<std::string, Mesh> _meshes;
    std::unordered_map<std::string, Model> _models;

    MeshManager(const Device& device, UploadContext& uploadContext);
    ~MeshManager();

    // void load_meshes();
    void upload_mesh(Mesh& mesh);
    void upload_mesh(Model& mesh);
    Mesh* get_mesh(const std::string &name);
    Model* get_model(const std::string &name);

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
    const class Device& _device;
    UploadContext& _uploadContext;

};