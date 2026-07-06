#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "SDL3/SDL_rect.h"
#include "flecs.h"
#include "flecs/addons/cpp/entity.hpp"
#include "glaze/glaze.hpp"

#include "meta/component_meta.hpp"

namespace atmo::core
{
    struct SignalQueue {
        static void SetWorld(flecs::world *w)
        {
            s_world = w;
        }

        static bool IsReadonly()
        {
            return s_world && s_world->is_readonly();
        }

        static void Enqueue(std::function<void()> fn)
        {
            s_pending.emplace_back(std::move(fn));
        }

        static void Flush()
        {
            while (!s_pending.empty()) {
                auto tasks = std::move(s_pending);
                s_pending.clear();
                for (auto &t : tasks) t();
            }
        }

        static inline flecs::world *s_world = nullptr;
        static inline std::vector<std::function<void()>> s_pending;
    };

    struct ISignal {
        virtual ~ISignal() = default;
        virtual std::type_index type() const = 0;
        virtual void disconnectAll() = 0;
    };

    template <typename... Args> class Signal : public ISignal
    {
    public:
        using Callback = std::function<void(Args...)>;

        std::type_index type() const override
        {
            return typeid(Signal<Args...>);
        }

        void connect(Callback cb)
        {
            m_callbacks.emplace_back(std::move(cb));
        }

        void disconnectAll() override
        {
            m_callbacks.clear();
        }

        void disconnectIndex(std::uint32_t index)
        {
            m_callbacks.erase(index);
        }

        void emit(Args... args)
        {
            if (SignalQueue::IsReadonly()) {
                SignalQueue::Enqueue([this, args...]() mutable {
                    for (auto &cb : m_callbacks) cb(args...);
                });
            } else {
                for (auto &cb : m_callbacks) cb(args...);
            }
        }

    private:
        std::vector<Callback> m_callbacks;
    };
} // namespace atmo::core

namespace atmo::core::components
{
    struct EntityBase {
        std::string type_name;
        std::unordered_map<std::string, ISignal *> signals;
    };

} // namespace atmo::core::components

namespace atmo::core::ecs::entities
{
    class Scene;

    struct EntityData {
        std::string name;
        std::string type;

        std::unordered_map<std::string, glz::generic> components;

        std::vector<EntityData> children;
    };

    class Entity
    {
    public:
        Entity() = default;
        Entity(const flecs::entity &handle) : p_handle(handle) {}
        Entity(const Entity &copy) : p_handle(copy.p_handle) {}
        virtual ~Entity() = default;

        operator flecs::entity() const
        {
            return p_handle;
        }

        bool operator==(const Entity &other) const
        {
            return p_handle == other.p_handle;
        }

        static constexpr std::string_view FullName()
        {
            return "Entity";
        }

        EntityData serialize() const;
        void deserializeJson(std::string_view data);
        void deserialize(const EntityData &data);

        /**
         * @brief Deserialize an EntityData into this entity, creating children inside @p world.
         *        Use this instead of deserialize() when this entity lives in an isolated (non-main) world.
         *
         * @param data Serialized entity data.
         * @param world The isolated world to create child entities in.
         */
        void deserializeInWorld(const EntityData &data, flecs::world *world);

        /**
         * @brief Set a component for the entity.
         *
         * @tparam Component Type of the component to set. Must not be a tag component (struct with no data).
         * @param component Component data to set for the entity. If the entity already has a component of this type, it will be replaced with the new data.
         */
        template <typename Component> void setComponent(Component &&component)
        {
            p_handle.set(std::forward<Component>(component));
        }

        /**
         * @brief Adds a tag component (struct with no data) to the entity.
         *
         * @tparam Component Type of the tag component to add. Must be a struct with no data.
         */
        template <typename Component> void addTag()
        {
            p_handle.add<Component>();
        }

        /**
         * @brief Method to register systems related to this entity type. Called once when world is initialized.
         *
         * @param world Pointer to the flecs world, can be used to register systems.
         */
        static void RegisterSystems(flecs::world *world);

        /**
         * @brief Method to cleanup any static resources related to this entity type. Called once when world is being cleaned up.
         *
         * @param world World that is being cleaned up.
         */
        static void Unregister(flecs::world *world);

        /**
         * @brief Method used to initialize the entity after it has been created. Can be used to set default components or do other setup tasks.
         *
         */
        void initialize();

        /**
         * @brief Get the Handle object
         *
         * @return flecs::entity
         */
        flecs::entity getHandle() const;

        /**
         * @brief Get all of this entity's children.
         *
         * @param bool Should the returned list contain children of children recursively?
         * @return std::vector<Entity> containing all of this entity's children.
         */
        std::vector<Entity> getChildren(bool recursive = false) const;

        /**
         * @brief Get the child of entity by name.
         *
         * @param name Name of the child entity.
         * @return Entity Child entity.
         */
        Entity getChild(std::string_view name) const;

        /**
         * @brief Set the parent of this entity.
         *
         * @param parent Parent entity to set for this entity.
         */
        void setParent(Entity parent)
        {
            p_handle.child_of(parent.p_handle);
        }

        /**
         * @brief Get the entity's parent entity.
         *
         * @return Entity parent entity.
         */
        template <typename ParentType = Entity> ParentType getParent() const
        {
            auto parent_handle = p_handle.parent();
            if (!parent_handle.is_valid())
                return ParentType();
            return ParentType(parent_handle);
        }

        /**
         * @brief Destroy the entity and remove it from scene.
         *
         */
        void destroy();

        /**
         * @brief Check if entity is valid and alive.
         *
         * @return true Entity is valid and alive.
         * @return false Entity is either invalid or destroyed.
         */
        bool isAlive() const;

        /**
         * @brief Get the entity's name.
         *
         * @return std::string_view Name of the entity.
         */
        std::string_view name() const;

        /**
         * @brief Set the entity's name.
         *
         * @param new_name New name for the entity.
         */
        void rename(const std::string &new_name);

        /**
         * @brief Check if this entity is a child of the given parent entity.
         *
         * @param parent Entity to check against.
         * @return true This entity is a child of the given parent.
         * @return false This entity is not a child of the given parent.
         */
        bool isChildOf(Entity parent);

        /**
         * @brief Get the Scene that the entity belongs to.
         *
         * @return std::shared_ptr<entities::Scene> Scene that the entity belongs to.
         */
        std::shared_ptr<entities::Scene> getScene() const;
        /**
         * @brief Returns the internal ID of the entity.
         *
         * @return std::uint64_t internal ID
         */
        std::uint64_t getID() const;

        /**
         * @brief Check if the entity has a component.
         *
         * @tparam Component component of the entity
         * @return true Entity has component
         * @return false Entity does not have component
         */
        template <typename Component> bool hasComponent() const
        {
            return p_handle.has<Component>();
        }

        /**
         * @brief Get the Component of the entity.
         *
         * @tparam Component to get.
         * @return const Component& read-only component of the entity.
         */
        template <typename Component> const Component &getComponent() const
        {
            return p_handle.get<Component>();
        }

        /**
         * @brief Get the Mutable Component of the entity.
         *
         * @tparam Component to get.
         * @return Component& component of the entity.
         */
        template <typename Component> Component &getComponentMutable()
        {
            return p_handle.get_mut<Component>();
        }

        /**
         * @brief Create a Signal definition for the entity.
         *
         * @tparam Args of the signal
         * @param id Key name of the signal
         * @return Signal<Args...>& Signal object with all connections
         */
        template <typename... Args> Signal<Args...> &createSignal(const std::string &id)
        {
            auto *ptr = new Signal<Args...>;
            auto &base_comp = getComponentMutable<components::EntityBase>();

            base_comp.signals[id] = ptr;

            return *ptr;
        }

        /**
         * @brief Get the Signal object definition of the entity.
         *
         * @tparam Args of the signal
         * @param id Key name of the signal
         * @return Signal<Args...>& Signal object with all connections
         */
        template <typename... Args> Signal<Args...> &getSignal(const std::string &id)
        {
            auto &base_comp = getComponentMutable<components::EntityBase>();
            auto it = base_comp.signals.find(id);
            assert(it != base_comp.signals.end());

            auto *base = it->second;

            assert(base->type() == typeid(Signal<Args...>));

            return *static_cast<Signal<Args...> *>(base);
        }

        /**
         * @brief Compute an axis-aligned bounding box for this entity in world space.
         *        Returns a zero rect by default; overridden by concrete 2D entity types.
         */
        virtual SDL_FRect computeAABB() const
        {
            return { 0.f, 0.f, 0.f, 0.f };
        }

        /**
         * @brief Swap the current entity with the dest Entity.
         *
         * @param dest dest Entity
         */
        void swap(const Entity &dest);

    protected:
        flecs::entity p_handle;
    };
} // namespace atmo::core::ecs::entities
