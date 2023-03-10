#pragma once

#include "glm/glm.hpp"
#include "core/manager/vk_system_manager.h"

#include <atomic>
#include <unordered_map>

uint32_t const MAX_LIGHT = 8;

struct GPULightData {
    alignas(uint32_t) uint32_t num_lights;
    alignas(16) glm::vec4 position[MAX_LIGHT];
    alignas(16) glm::vec4 color[MAX_LIGHT];
    // float cut_off;
};

class Light : public Entity {
protected:
    static std::atomic<uint32_t> nextID;

public:
    enum Type {
        POINT = 0,
        SPOT = 1,
        DIRECTIONAL = 2
    };
    static inline std::unordered_map<Type, const char*> types = {
            {POINT, "Point"},
            {SPOT, "Spot"},
            {DIRECTIONAL, "Directional"}
    };

    uint32_t _uid {0};

    Light();
    Light(Type type, glm::vec4 pos, glm::vec4 color);

    Type get_type();
    glm::vec4 get_position();
    glm::vec4 get_color();

    void set_type(Type type);
    void set_position(glm::vec4 p);
    void set_color(glm::vec4 c);

private:

    Type _type = Type::POINT;
    glm::vec4 _position = {0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 _color {1.0f, 1.0f, 1.0f, 1.0f};
};

class LightingManager : public System {
public:
    GPULightData gpu_format();
};