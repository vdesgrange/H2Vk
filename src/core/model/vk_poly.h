#pragma once

#include "vk_model.h"

class Device;
class Model;

class ModelPOLY final: public Model {
public:
    using Model::Model;

    static std::shared_ptr<Model> create_cube(Device* device, const glm::vec3& p0, const glm::vec3& p1);
    static std::shared_ptr<Model> create_triangle(Device* device, glm::vec3 color = { 1.f, 1.f, 1.f});
    static std::shared_ptr<Model> create_uv_sphere(Device* device, const glm::vec3& center, float radius = 1.f, const uint32_t stacks = 16, const uint32_t sectors = 17, glm::vec3 color = {1.f, 1.f, 1.f});
    static std::shared_ptr<Model> create_plane(Device* device, const glm::vec3& p0, const glm::vec3& p1, glm::vec3 color = {1.f, 1.f, 1.f});
};