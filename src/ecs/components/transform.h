#pragma once

#include "glm/glm.hpp"

struct TransformComponent {
    alignas(glm::vec4) glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    alignas(glm::vec4) glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    alignas(glm::vec4) glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};