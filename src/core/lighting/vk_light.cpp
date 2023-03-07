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