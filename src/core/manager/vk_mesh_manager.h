/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "VkBootstrap.h"
#include "core/manager/vk_system_manager.h"

class Device;
class Mesh;
class Model;
class Fence;
class CommandPool;
class CommandBuffer;
class UploadContext;

class MeshManager : public System {
public:
    MeshManager(const Device* device, UploadContext* uploadContext);
    ~MeshManager();

    void upload_mesh(Model& mesh);
    std::shared_ptr<Model> get_model(const std::string &name);

private:
    const class Device* _device;
    UploadContext* _uploadContext;
};