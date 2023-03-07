#pragma once

#include "vk_mem_alloc.h"
#include "glm/glm.hpp"
#include "core/vk_shaders.h"

#include <deque>
#include <functional>

struct GPUSceneData {
    glm::vec4 fogColor;
    glm::vec4 fogDistance;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
    glm::float32 specularFactor;
};

struct GPUObjectData {
    glm::mat4 model;
};

class Model;
struct RenderObject {
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> material;
    glm::mat4 transformMatrix;
};

typedef std::vector<RenderObject> Renderables;

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
            (*it)(); // call the function
        }

        deletors.clear();
    }
};