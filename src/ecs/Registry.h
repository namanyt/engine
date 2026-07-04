#pragma once

#include "ecs/Entity.h"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::ecs
{
/**
 * @brief Entity-Component registry with type-erased component storage.
 *
 * The `Registry` is the central data structure for the ECS subsystem.
 * It manages entity lifecycle (create/destroy), component storage,
 * and query iteration via `forEach<...>()`.
 *
 * Components are stored in per-type `Storage<T>` instances, created
 * lazily on first `emplace`. The registry tracks alive entities and
 * removes all components when an entity is destroyed.
 *
 * @par Example
 * @code
 * Registry registry;
 * auto e = registry.createEntity();
 * registry.emplace<Position>(e, Vec3{1.0f, 0.0f, 0.0f});
 * registry.emplace<Velocity>(e, Vec3{0.0f, 0.0f, 1.0f});
 *
 * registry.forEach<Position, Velocity>([](Entity entity, Position& pos, Velocity& vel) {
 *     pos += vel;
 * });
 * @endcode
 *
 * @see Entity
 * @see components::TransformComponent
 */
class Registry final
{
  public:
    /// @brief Creates a new entity and returns its handle.
    /// @return A unique Entity ID (sequential, starting from 1).
    Entity createEntity()
    {
        const Entity entity = m_nextEntity++;
        m_aliveEntities.push_back(entity);
        return entity;
    }

    /// @brief Destroys an entity and removes all its components.
    /// @param entity The entity handle to destroy.
    /// @note No-op if the entity is already dead or invalid.
    void destroyEntity(Entity entity)
    {
        if (!isAlive(entity))
        {
            return;
        }

        for (auto& [_, storage] : m_componentStorages)
        {
            storage->remove(entity);
        }

        m_aliveEntities.erase(std::remove(m_aliveEntities.begin(), m_aliveEntities.end(), entity),
                              m_aliveEntities.end());
    }

    /// @brief Checks whether an entity is still alive.
    /// @param entity The entity handle to check.
    /// @return true if the entity exists and has not been destroyed.
    bool isAlive(Entity entity) const
    {
        return entity != kInvalidEntity && std::find(m_aliveEntities.begin(), m_aliveEntities.end(),
                                                     entity) != m_aliveEntities.end();
    }

    /// @brief Destroys all entities and clears all component storage.
    void clear()
    {
        m_componentStorages.clear();
        m_aliveEntities.clear();
        m_nextEntity = 1;
    }

    /// @brief Returns the number of alive entities.
    /// @return Count of active entities in the registry.
    std::size_t entityCount() const noexcept
    {
        return m_aliveEntities.size();
    }

    /// @brief Returns the number of distinct component types registered.
    /// @return Count of unique component type storages.
    std::size_t componentTypeCount() const noexcept
    {
        return m_componentStorages.size();
    }

    /// @brief Returns the total number of component instances across all types.
    /// @return Sum of all component storage sizes.
    std::size_t totalComponentCount() const noexcept
    {
        std::size_t total = 0;
        for (const auto& [_, storage] : m_componentStorages)
        {
            total += storage->size();
        }

        return total;
    }

    /// @brief Emplaces a component on an entity, constructing it in-place.
    /// @tparam Component The component type to add.
    /// @tparam Args Constructor arguments forwarded to the component.
    /// @param entity Target entity handle.
    /// @param args Forwarded constructor arguments.
    /// @return Reference to the newly constructed component.
    /// @throws std::runtime_error if the entity is not alive.
    template <typename Component, typename... Args>
    Component& emplace(Entity entity, Args&&... args)
    {
        static_assert(std::is_same_v<Component, std::decay_t<Component>>,
                      "Component type must not include cv/ref qualifiers.");
        if (!isAlive(entity))
        {
            throw std::runtime_error("Cannot add a component to a dead ECS entity.");
        }

        return assureStorage<Component>().emplace(entity, std::forward<Args>(args)...);
    }

    /// @brief Removes a component from an entity (no-op if not present).
    /// @tparam Component The component type to remove.
    /// @param entity Target entity handle.
    template <typename Component> void remove(Entity entity)
    {
        if (Storage<Component>* storage = findStorage<Component>(); storage != nullptr)
        {
            storage->remove(entity);
        }
    }

    /// @brief Checks whether an entity has a specific component type.
    /// @tparam Component The component type to check.
    /// @param entity Target entity handle.
    /// @return true if the entity has the component.
    template <typename Component> bool has(Entity entity) const
    {
        if (const Storage<Component>* storage = findStorage<Component>(); storage != nullptr)
        {
            return storage->has(entity);
        }

        return false;
    }

    /// @brief Retrieves a mutable reference to a component.
    /// @tparam Component The component type to retrieve.
    /// @param entity Target entity handle.
    /// @return Reference to the component instance.
    /// @throws std::runtime_error if the component is not present.
    template <typename Component> Component& get(Entity entity)
    {
        if (Component* component = tryGet<Component>(entity); component != nullptr)
        {
            return *component;
        }

        throw std::runtime_error("Requested ECS component is not present on the entity.");
    }

    /// @brief Retrieves a const reference to a component.
    /// @tparam Component The component type to retrieve.
    /// @param entity Target entity handle.
    /// @return Const reference to the component instance.
    /// @throws std::runtime_error if the component is not present.
    template <typename Component> const Component& get(Entity entity) const
    {
        if (const Component* component = tryGet<Component>(entity); component != nullptr)
        {
            return *component;
        }

        throw std::runtime_error("Requested ECS component is not present on the entity.");
    }

    /// @brief Attempts to retrieve a mutable pointer to a component.
    /// @tparam Component The component type to retrieve.
    /// @param entity Target entity handle.
    /// @return Pointer to the component, or nullptr if not present.
    template <typename Component> Component* tryGet(Entity entity)
    {
        if (Storage<Component>* storage = findStorage<Component>(); storage != nullptr)
        {
            return storage->tryGet(entity);
        }

        return nullptr;
    }

    /// @brief Attempts to retrieve a const pointer to a component.
    /// @tparam Component The component type to retrieve.
    /// @param entity Target entity handle.
    /// @return Pointer to the component, or nullptr if not present.
    template <typename Component> const Component* tryGet(Entity entity) const
    {
        if (const Storage<Component>* storage = findStorage<Component>(); storage != nullptr)
        {
            return storage->tryGet(entity);
        }

        return nullptr;
    }

    /// @brief Iterates over all entities that have the specified component types.
    /// @tparam Components Pack of component types to query.
    /// @tparam Function Callable signature: `void(Entity, Component1&, Component2&, ...)`.\n
    /// @param function Callback invoked for each matching entity.
    template <typename... Components, typename Function> void forEach(Function&& function)
    {
        static_assert(sizeof...(Components) > 0, "forEach requires at least one component type.");

        IStorage* smallestStorage = nullptr;
        ((smallestStorage = pickSmallerStorage(smallestStorage, findStorage<Components>())), ...);

        if (smallestStorage == nullptr)
        {
            return;
        }

        for (Entity entity : smallestStorage->entities())
        {
            if ((has<Components>(entity) && ...))
            {
                function(entity, get<Components>(entity)...);
            }
        }
    }

    /// @brief Const overload: iterates over all entities with the specified component types.
    /// @tparam Components Pack of component types to query.
    /// @tparam Function Callable signature: `void(Entity, Component1&, Component2&, ...)`.\n
    /// @param function Callback invoked for each matching entity.
    template <typename... Components, typename Function> void forEach(Function&& function) const
    {
        static_assert(sizeof...(Components) > 0, "forEach requires at least one component type.");

        const IStorage* smallestStorage = nullptr;
        ((smallestStorage = pickSmallerStorage(smallestStorage, findStorage<Components>())), ...);

        if (smallestStorage == nullptr)
        {
            return;
        }

        for (Entity entity : smallestStorage->entities())
        {
            if ((has<Components>(entity) && ...))
            {
                function(entity, get<Components>(entity)...);
            }
        }
    }

    /// @brief Returns the number of entities that have a specific component type.
    /// @tparam Component The component type to count.
    /// @return Number of entities with the component (0 if no storage exists).
    template <typename Component> std::size_t count() const noexcept
    {
        if (const Storage<Component>* storage = findStorage<Component>(); storage != nullptr)
        {
            return storage->size();
        }

        return 0;
    }

  private:
    struct IStorage
    {
        virtual ~IStorage() = default;
        virtual void remove(Entity entity) = 0;
        virtual bool has(Entity entity) const = 0;
        virtual std::size_t size() const noexcept = 0;
        virtual const std::vector<Entity>& entities() const noexcept = 0;
    };

    template <typename Component> class Storage final : public IStorage
    {
      public:
        template <typename... Args> Component& emplace(Entity entity, Args&&... args)
        {
            if (!has(entity))
            {
                m_entities.push_back(entity);
            }

            auto [iterator, inserted] =
                m_components.emplace(entity, Component{std::forward<Args>(args)...});
            if (!inserted)
            {
                iterator->second = Component{std::forward<Args>(args)...};
            }

            return iterator->second;
        }

        void remove(Entity entity) override
        {
            m_components.erase(entity);
            m_entities.erase(std::remove(m_entities.begin(), m_entities.end(), entity),
                             m_entities.end());
        }

        bool has(Entity entity) const override
        {
            return m_components.find(entity) != m_components.end();
        }

        Component* tryGet(Entity entity)
        {
            auto iterator = m_components.find(entity);
            return iterator != m_components.end() ? &iterator->second : nullptr;
        }

        const Component* tryGet(Entity entity) const
        {
            auto iterator = m_components.find(entity);
            return iterator != m_components.end() ? &iterator->second : nullptr;
        }

        std::size_t size() const noexcept override
        {
            return m_components.size();
        }

        const std::vector<Entity>& entities() const noexcept override
        {
            return m_entities;
        }

      private:
        std::unordered_map<Entity, Component> m_components;
        std::vector<Entity> m_entities;
    };

    template <typename Component> Storage<Component>& assureStorage()
    {
        const std::type_index typeIndex = std::type_index(typeid(Component));
        auto iterator = m_componentStorages.find(typeIndex);
        if (iterator == m_componentStorages.end())
        {
            iterator =
                m_componentStorages.emplace(typeIndex, std::make_unique<Storage<Component>>())
                    .first;
        }

        return static_cast<Storage<Component>&>(*iterator->second);
    }

    template <typename Component> Storage<Component>* findStorage()
    {
        const std::type_index typeIndex = std::type_index(typeid(Component));
        auto iterator = m_componentStorages.find(typeIndex);
        return iterator != m_componentStorages.end()
                   ? static_cast<Storage<Component>*>(iterator->second.get())
                   : nullptr;
    }

    template <typename Component> const Storage<Component>* findStorage() const
    {
        const std::type_index typeIndex = std::type_index(typeid(Component));
        auto iterator = m_componentStorages.find(typeIndex);
        return iterator != m_componentStorages.end()
                   ? static_cast<const Storage<Component>*>(iterator->second.get())
                   : nullptr;
    }

    static IStorage* pickSmallerStorage(IStorage* current, IStorage* candidate)
    {
        if (candidate == nullptr)
        {
            return current;
        }

        if (current == nullptr || candidate->size() < current->size())
        {
            return candidate;
        }

        return current;
    }

    static const IStorage* pickSmallerStorage(const IStorage* current, const IStorage* candidate)
    {
        if (candidate == nullptr)
        {
            return current;
        }

        if (current == nullptr || candidate->size() < current->size())
        {
            return candidate;
        }

        return current;
    }

    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> m_componentStorages;
    std::vector<Entity> m_aliveEntities;
    Entity m_nextEntity = 1;
};
} // namespace engine::ecs
