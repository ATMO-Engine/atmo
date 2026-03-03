#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "flecs.h"

namespace atmo::core::ecs::entities
{
    class Scene;

    class Entity
    {
    public:
        Entity() = default;
        Entity(const flecs::entity &handle) : p_handle(handle) {}

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

        template <typename Component> void setComponent(Component &&component)
        {
            p_handle.set(std::forward<Component>(component));
        }

        /**
         * @brief Method to register components related to this entity type. Called once when world is initialized before RegisterSystems.
         *
         * @param world
         */
        static void RegisterComponents(flecs::world *world);

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
         * @return std::vector<Entity> containing all of this entity's children.
         */
        std::vector<Entity> getChildren();

        /**
         * @brief Get the child of entity by name.
         *
         * @param name Name of the child entity.
         * @return Entity Child entity.
         */
        Entity getChild(std::string_view name);

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
         * @brief Load the entity's data from a JSON string.
         *
         * @param json_data JSON string containing the entity's data. The format of the JSON should match the structure of the entity and its components.
         */
        void loadFromJson(std::string_view json_data);

        /**
         * @brief Check if this entity is a child of the given parent entity.
         *
         * @param parent Entity to check against.
         * @return true This entity is a child of the given parent.
         * @return false This entity is not a child of the given parent.
         */
        bool isChildOf(Entity parent);

        // FIXME: Find a way to resolve recursive inclusion between Entity and Entity::Scene

        /**
         * @brief Get the Scene that the entity belongs to.
         *
         * @return std::shared_ptr<entities::Scene> Scene that the entity belongs to.
         */
        std::shared_ptr<entities::Scene> getScene() const;

    protected:
        flecs::entity p_handle;
    };
} // namespace atmo::core::ecs::entities
