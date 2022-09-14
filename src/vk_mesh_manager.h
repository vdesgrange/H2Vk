#pragma once

#include <string>
#include <unordered_map>

#include "VkBootstrap.h"


class Device;
class Mesh;
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

    MeshManager(const Device& device, UploadContext& uploadContext);
    ~MeshManager();

    void load_meshes();
    void upload_mesh(Mesh& mesh);
    Mesh* get_mesh(const std::string &name);

private:
    const class Device& _device;
    UploadContext& _uploadContext;

    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
};