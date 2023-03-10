#pragma once

#include <unordered_map>
#include <memory>
#include <string>

class Entity {
protected:
    static std::atomic<uint32_t> nextGUID;

public:
    uint32_t _guid{0}; // global unique id

    Entity() : _guid(++nextGUID) {};
};

class System {
public:
    std::unordered_map<std::string, std::shared_ptr<Entity>> _entities {};

    void add_entity(std::string name, std::shared_ptr<Entity> entity);
    std::shared_ptr<Entity> get_entity(const std::string name);

    void clear_entities();
};

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