/*
*  SystemManager, System and Entity
*
* Copyright (C) 2022-2023 by Viviane Desgrange
*
* This code is licensed under the Non-Profit Open Software License ("Non-Profit OSL") 3.0 (https://opensource.org/license/nposl-3-0/)
*/

#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <atomic>

/**
 * Entity - a unique ID
 */
class Entity {
protected:
    static std::atomic<uint32_t> nextGUID;

public:
    uint32_t _guid{0}; // global unique id

    Entity();
};

/**
 * System - logic
 * @brief Generic resource manager
 * @note To be updated to handle component logic in ECS
 */
class System {
public:
    std::unordered_map<std::string, std::shared_ptr<Entity>> _entities {};

    void add_entity(std::string name, std::shared_ptr<Entity> entity);
    std::shared_ptr<Entity> get_entity(const std::string name);

    void clear_entities();
};

/**
 * Register and handle generic managers (ie. lights, materials, meshes)
 * @brief Manager of managers
 * @note To be extended to an Entity-Component-System
 */
class SystemManager final {
public:

    template<typename T, typename... Args>
    std::shared_ptr<T> register_system(Args... args) {
        std::string name = typeid(T).name();
        std::shared_ptr<T> system = std::make_shared<T>(args...);

        _systems.insert({name, system});

        return system;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<System>> _systems {};
};