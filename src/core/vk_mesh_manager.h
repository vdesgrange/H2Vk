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
class UploadContext;

class MeshManager final {
public:
    std::unordered_map<std::string, std::shared_ptr<Model>> _models {}; // unique_ptr?

    MeshManager(const Device& device, UploadContext& uploadContext);
    ~MeshManager();

    void upload_mesh(Model& mesh);
    std::shared_ptr<Model> get_model(const std::string &name);  // Model* ?

private:
    const class Device& _device;
    UploadContext& _uploadContext;
};