#include "vk_system_manager.h"

#include <stdexcept>

std::atomic<uint32_t> Entity::nextGUID {0};

Entity::Entity() : _guid(++nextGUID) {}

void System::add_entity(std::string name, std::shared_ptr<Entity> entity) {
    _entities.emplace(name, entity);
}

std::shared_ptr<Entity> System::get_entity(const std::string name) {
    try {
        return _entities.at(name);
    } catch (const std::out_of_range& e) {
        return nullptr;
    }
}

void System::clear_entities() {
    _entities.clear();
}