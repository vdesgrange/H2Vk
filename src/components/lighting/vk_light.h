/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include "glm/glm.hpp"
#include "core/manager/vk_system_manager.h"
#include "core/utilities/vk_resources.h"

#include <atomic>
#include <unordered_map>

uint32_t const MAX_LIGHT = 8;

struct GPULightData {
    alignas(8) glm::vec2 num_lights;
    alignas(16) glm::vec4 dirDirection[MAX_LIGHT];
    alignas(16) glm::vec4 dirColor[MAX_LIGHT];
    alignas(16) glm::vec4 spotPosition[MAX_LIGHT];
    alignas(16) glm::vec4 spotDirection[MAX_LIGHT];
    alignas(16) glm::vec4 spotColor[MAX_LIGHT];
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
    Light(Type type, glm::vec4 p, glm::vec4 r, glm::vec4 c);
    Light(glm::vec4 p, glm::vec4 r, glm::vec4 c);
    Light(glm::vec4 r, glm::vec4 c);

    Type get_type();
    glm::vec4 get_position();
    glm::vec4 get_rotation();
    glm::vec4 get_color();

    void set_type(Type type);
    void set_position(glm::vec4 p);
    void set_rotation(glm::vec4 t);
    void set_color(glm::vec4 c);

private:

    Type _type = Type::SPOT;
    glm::vec4 _position = {0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 _rotation = {0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 _color = {1.0f, 1.0f, 1.0f, 1.0f};
};

class LightingManager final : public System {
public:
    GPULightData gpu_format();

    static void allocate_buffers(Device& device);
};