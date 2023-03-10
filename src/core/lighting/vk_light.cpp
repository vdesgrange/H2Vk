#include "vk_light.h"

std::atomic<uint32_t> Light::nextID {0};

Light::Light() : _uid(++nextID) {}

Light::Light(Type type, glm::vec4 pos, glm::vec4 color) {
    _uid = ++nextID;
    _type = type;
    _position = pos;
    _color = color;
}

Light::Type Light::get_type() {
    return this->_type;
}

glm::vec4 Light::get_position() {
    return this->_position;
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

void Light::set_color(glm::vec4 c) {
    this->_color = c;
}

GPULightData LightingManager::gpu_format() {
    GPULightData lightingData{};

    lightingData.num_lights = static_cast<uint32_t>(this->_entities.size());
    int lightCount = 0;
    for (auto& l : this->_entities) {
        std::shared_ptr<Light> light = std::static_pointer_cast<Light>(l.second);
        lightingData.position[lightCount] = light->get_position();
        lightingData.color[lightCount] = light->get_color();
        lightCount++;
    }

    return lightingData;
}