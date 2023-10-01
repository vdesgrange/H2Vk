/*
*  H2Vk - A Vulkan based rendering engine
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#include "vk_light.h"
#include "core/utilities/vk_global.h"

std::atomic<uint32_t> Light::nextID {0};

Light::Light() : _uid(++nextID) {}

Light::Light(Type type, glm::vec4 p, glm::vec4 r, glm::vec4 c) {
    _uid = ++nextID;
    _type = type;
    _position = p;
    _rotation = r;
    _color = c;
}

Light::Light(glm::vec4 p, glm::vec4 r, glm::vec4 c) {
    _uid = ++nextID;
    _type = Light::SPOT;
    _position = p;
    _rotation = r;
    _color = c;
}

Light::Light(glm::vec4 r, glm::vec4 c) {
    _uid = ++nextID;
    _type = Light::DIRECTIONAL;
    _position = glm::vec4(0.0f);
    _rotation = r;
    _color = c;
}


Light::Type Light::get_type() {
    return this->_type;
}

glm::vec4 Light::get_position() {
    return this->_position;
}

glm::vec4 Light::get_rotation() {
    return this->_rotation;
}

glm::vec4 Light::get_color() {
    return this->_color;
}

void Light::set_type(Light::Type type) {
    this->_type = type;
}

void Light::set_position(glm::vec4 p) {
    this->_position = p;
}

void Light::set_rotation(glm::vec4 r) {
    this->_rotation = r;
}

void Light::set_color(glm::vec4 c) {
    this->_color = c;
}

GPULightData LightingManager::gpu_format() {
    GPULightData lightingData{};

    int dirLightCount = 0;
    int spotLightCount = 0;

    for (auto& l : this->_entities) {
        std::shared_ptr<Light> light = std::static_pointer_cast<Light>(l.second);

        if (light->get_type() == Light::Type::DIRECTIONAL) {
            lightingData.dirDirection[dirLightCount] =  light->get_rotation();
            lightingData.dirColor[dirLightCount] = light->get_color();
            dirLightCount++;
        }

       if (light->get_type() == Light::Type::SPOT) {
           lightingData.spotPosition[spotLightCount] = light->get_position();
           lightingData.spotDirection[spotLightCount] = light->get_rotation();
           lightingData.spotColor[spotLightCount] = light->get_color();
           spotLightCount++;
       }
    }

    lightingData.num_lights = glm::vec2(spotLightCount, dirLightCount);


    return lightingData;
}

void LightingManager::allocate_buffers(Device& device) {
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        const size_t lightBufferSize = helper::pad_uniform_buffer_size(device, sizeof(GPULightData)); // FRAME_OVERLAP * 
        g_frames[i].lightingBuffer = Buffer::create_buffer(device, lightBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
}