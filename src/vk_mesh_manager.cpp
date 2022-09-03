#include "vk_mesh_manager.h"
#include "vk_helpers.h"
#include "vk_device.h"
#include "vk_mesh.h"

MeshManager::MeshManager(const Device& device) : _device(device) {
    load_meshes();
};

MeshManager::~MeshManager() {
    for (auto& it: _meshes) {
        vmaDestroyBuffer(_device._allocator,
                         it.second._vertexBuffer._buffer,
                         it.second._vertexBuffer._allocation);
    }
}

void MeshManager::load_meshes()
{
    Mesh mesh{};
    Mesh objMesh{};
    mesh._vertices.resize(3);

    mesh._vertices[0].position = { 1.f, 1.f, 0.0f };
    mesh._vertices[1].position = {-1.f, 1.f, 0.0f };
    mesh._vertices[2].position = { 0.f,-1.f, 0.0f };

    mesh._vertices[0].color = { 0.f, 1.f, 0.0f }; //pure green
    mesh._vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
    mesh._vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green

    objMesh.load_from_obj("../assets/monkey_smooth.obj");

    upload_mesh(mesh);
    upload_mesh(objMesh);

    _meshes["monkey"] = objMesh;
    _meshes["triangle"] = mesh;
}

void MeshManager::upload_mesh(Mesh& mesh)
{
    //allocate vertex buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    //this is the total size, in bytes, of the buffer we are allocating
    bufferInfo.size = mesh._vertices.size() * sizeof(Vertex);
    //this buffer is going to be used as a Vertex Buffer
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaallocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    //allocate the buffer
    VkResult result = vmaCreateBuffer(_device._allocator, &bufferInfo, &vmaallocInfo,
                                      &mesh._vertexBuffer._buffer,
                                      &mesh._vertexBuffer._allocation,
                                      nullptr);
    VK_CHECK(result);

    //add the destruction of triangle mesh buffer to the deletion queue

    void* data;
    vmaMapMemory(_device._allocator, mesh._vertexBuffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
    vmaUnmapMemory(_device._allocator, mesh._vertexBuffer._allocation);
}

Mesh* MeshManager::get_mesh(const std::string &name) {
    auto it = _meshes.find(name);
    if ( it == _meshes.end()) {
        return nullptr;
    } else {
        return &(*it).second;
    }
}
