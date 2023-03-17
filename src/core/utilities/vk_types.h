﻿#pragma once

#include "glm/glm.hpp"
#include "core/vk_shaders.h"

#include <deque>
#include <functional>

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