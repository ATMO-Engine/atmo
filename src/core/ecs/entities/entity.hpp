#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "flecs.h"
#include "glaze/glaze.hpp"


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
        void deserialize(std::string_view data);

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
        Entity getParent();

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
        bool isAlive();

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

        template <typename Component> const Component &getComponent() const
        {
            return p_handle.get<Component>();
        }

        template <typename Component> Component &getComponentMutable()
        {
            return p_handle.get_mut<Component>();
        }

    protected:
        flecs::entity p_handle;
    };
} // namespace atmo::core::ecs::entities

namespace atmo::core::components
{
    struct EntityType {
        std::string type_name;
    };
} // namespace atmo::core::components
