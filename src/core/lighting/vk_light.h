#pragma once

#include "glm/glm.hpp"

uint32_t const MAX_LIGHT = 8;

struct GPULightData {
    alignas(int32_t) int32_t num_lights;
    alignas(glm::vec4) glm::vec4 position[MAX_LIGHT];
    alignas(MAX_LIGHT * alignof(glm::vec4)) glm::vec4 color[MAX_LIGHT];
    // float cut_off;
};

class Light final {
public:
    enum Type {
        POINT = 0,
        SPOT = 1,
        DIRECTIONAL = 2
    };

    Type _type = Type::POINT;
    glm::vec4 _position = {0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 _color {1.0f, 1.0f, 1.0f, 1.0f};
};