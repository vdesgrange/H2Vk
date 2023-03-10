#include "vk_system_manager.h"

std::atomic<uint32_t> Entity::nextGUID {0};

void System::add_entity(std::string name, std::shared_ptr<Entity> entity) {
    _entities.emplace(name, entity);
}

std::shared_ptr<Entity> System::get_entity(const std::string name) {
    try {
        return _entities.at(name);
    } catch (const std::out_of_range&) {
        return nullptr;
    }

//    auto it = _entities.find(name);
//    if ( it == _entities.end()) {
//        return nullptr;
//    } else {
//        return it->second;
//    }
}