#pragma once

#include <string>
#include <unordered_map>

class Device;
class Mesh;

class MeshManager final {
public:
    std::unordered_map<std::string, Mesh> _meshes;

    MeshManager(const Device& device);
    ~MeshManager();

    void load_meshes();
    void upload_mesh(Mesh& mesh);
    Mesh* get_mesh(const std::string &name);

private:
    const class Device& _device;
};