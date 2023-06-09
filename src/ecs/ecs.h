#pragma once

#include <cstdint>
#include <bitset>
#include <queue>
#include <array>
#include <unordered_map>
#include <set>
#include <assert.h>

using Entity = uint32_t;
using ComponentType = std::uint8_t;
const Entity MAX_ENTITIES = 512;
const ComponentType MAX_COMPONENTS = 32;

using Signature = std::bitset<MAX_COMPONENTS>;


class EntityManager {
public:
    EntityManager() {
        for (Entity e = 0; e < MAX_ENTITIES; e++) {
            free_entities.push(e);
        }
    }

    Entity create_entity() {
        assert(entity_count < MAX_ENTITIES && "Maximum number of entities reached.");

        Entity uid = free_entities.front();
        free_entities.pop();
        entity_count++;
        return uid;
    }

    void destroy_entity(Entity entity) {
        assert(entity_count < MAX_ENTITIES && "Entity out of range.");

        entity_signature[entity].reset();
        free_entities.push(entity);
        entity_count--;
    }

    void set_signature(Entity entity, Signature signature) {
        assert(entity_count < MAX_ENTITIES && "Entity out of range.");

        entity_signature[entity] = signature;
    }

    Signature get_signature(Entity entity) {
        assert(entity_count < MAX_ENTITIES && "Entity out of range.");
        return entity_signature[entity];
    }

private:
    uint32_t entity_count = 0;
    std::queue<Entity> free_entities {};
    std::array<Signature, MAX_ENTITIES> entity_signature {};
};

class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void entity_destroyed(Entity entity) = 0;
};

template<typename T>
class ComponentArray : IComponentArray {
public:
    void add_component(Entity entity, T component) {
        assert(entity_to_index.find(entity) == entity_to_index.end() && "Component already added.");

        uint32_t index = component_count;
        entity_to_index[entity] = index;
        index_to_entity[index] = entity;
        components[index] = component;

        component_count++;
    }

    void remove_component(Entity entity) {
        assert(entity_to_index.find(entity) != entity_to_index.end() && "Entity not found. Component cannot be removed.");

        uint32_t index = entity_to_index[entity];
        uint32_t last_index = component_count - 1;
        components[index] = components[last_index];
        Entity last = index_to_entity[last_index];
        entity_to_index[last] = index;
        index_to_entity[index] = last;

        entity_to_index.erase(entity);
        index_to_entity.erase(last_index);

        component_count--;
    }

    T& get_component(Entity entity) {
        assert(entity_to_index.find(entity) != entity_to_index.end() && "Entity not found. Component cannot be removed.");

        return components[entity_to_index[entity]];
    }

    void entity_destroyed(Entity entity) override {
        if (entity_to_index.find(entity) != entity_to_index.end()) {
            remove_component(entity);
        }
    }

private:
    std::array<T, MAX_ENTITIES> components;
    std::unordered_map<Entity, uint32_t> entity_to_index;
    std::unordered_map<uint32_t, uint32_t> index_to_entity;
    uint32_t component_count;
};

class ComponentManager {
public:
    template<typename T>
    void register_component() {
        const char* type_name = typeid(T).name();

        assert(component_types.find(type_name) == component_types.end() && "Component type to be registered more than once.");

        component_types.insert({type_name, utid});
        component_arrays.insert({type_name, std::make_shared<ComponentArray<T>>});

        utid++;
    }

    template<typename T>
    ComponentType get_component_type() {
        const char* type_name = typeid(T).name();
        assert(component_types.find(type_name) != component_types.end() && "Component type not registered.");

        return component_types[type_name];
    }

    template<typename T>
    void add_component(Entity entity, T component) {
        get_component_array<T>()->add_component(entity, component);
    }

    template<typename T>
    void remove_component(Entity entity) {
        get_component_array<T>()->remove_component(entity);
    }

    template<typename T>
    T& get_component(Entity entity) {
        return get_component_array<T>()->get_component(entity);
    }

    void entity_destroyed(Entity entity) {
        for (auto const& p : component_arrays) {
            auto const& component = p.second;
            component->entity_destroyed(entity);
        }
    }

private:
    ComponentType utid = 0; // unique type id
    std::unordered_map<const char*, ComponentType> component_types {};
    std::unordered_map<const char*, std::shared_ptr<IComponentArray>> component_arrays {};

    template <typename T>
    std::shared_ptr<ComponentArray<T>> get_component_array() {
        const char* type_name = typeid(T).name();

        assert(component_types.find(type_name) != component_types.end() && "Component not registered.");

        return std::static_pointer_cast<ComponentArray<T>>(component_arrays[type_name]);
    }
};

class System {
public:
    std::set<Entity> entities;
};

class SystemManager {
public:
    template <typename T>
    std::shared_ptr<T> register_system() {
        const char* type_name = typeid(T).name();
        assert(systems.find(type_name) == systems.end() && "System is already registered.");

        auto system = std::make_shared<T>();
        systems.insert({type_name, system});

        return system;
    }

    template <typename T>
    void set_signature(Signature signature) {
        const char* type_name = typeid(T).name();

        assert(systems.find(type_name) != systems.end() && "System must be registered.");

        signatures.insert({type_name, signature});
    }

    void entity_destroyed(Entity entity) {
        for (auto const& p : systems) {
            auto const& system = p.second;
            system->entities.erase(entity);
        }
    }

    void entity_signature_changed(Entity entity, Signature signature) {
        for (auto const& p : systems) {
            auto const& type = p.first;
            auto const& system = p.second;
            auto const& sys_signature = signatures[type];

            if ((signature & sys_signature) == sys_signature) {
                system->entities.insert(entity);
            } else {
                system->entities.erase(entity);
            }
        }
    }

private:
    std::unordered_map<const char*, std::shared_ptr<System>> systems {};
    std::unordered_map<const char*, Signature> signatures {};
};

class Coordinator {
public:

    void init() {
        _entityManager = std::make_unique<EntityManager>();
        _componentManager = std::make_unique<ComponentManager>();
        _systemManager = std::make_unique<SystemManager>();
    }

    Entity create_entity() {
        return _entityManager->create_entity();
    }

    void destroy_entity(Entity entity) {
        _entityManager->destroy_entity(entity);
        _componentManager->entity_destroyed(entity);
        _systemManager->entity_destroyed(entity);
    }

    template <typename T>
    void register_component() {
        _componentManager->register_component<T>();
    }

    template <typename T>
    void add_component(Entity entity, T component) {
        _componentManager->add_component(entity, component);

        auto signature = _entityManager->get_signature(entity);
        signature.set(_componentManager->get_component_type<T>(), true);
        _entityManager->set_signature(entity, signature);

        _systemManager->entity_signature_changed(entity, signature);
    }

    template <typename T>
    void remove_component(Entity entity) {
        _componentManager->remove_component<T>(entity);

        auto signature = _entityManager->get_signature(entity);
        signature.set(_componentManager->get_component_type<T>(), false);
        _entityManager->set_signature(entity, signature);

        _systemManager->entity_signature_changed(entity, signature);
    }

    template <typename T>
    T& get_component(Entity entity) {
        return _componentManager->get_component<T>(entity);
    }

    template <typename T>
    ComponentType get_component_type(Entity entity) {
        return _componentManager->get_component_type<T>();
    }

    template <typename T>
    std::shared_ptr<T> register_system() {
        return _systemManager->register_system<T>();
    }

    template <typename T>
    void set_system_signature(Signature signature) {
        _systemManager->set_signature<T>(signature);
    }

private:
    std::unique_ptr<EntityManager> _entityManager;
    std::unique_ptr<ComponentManager> _componentManager;
    std::unique_ptr<SystemManager> _systemManager;
};