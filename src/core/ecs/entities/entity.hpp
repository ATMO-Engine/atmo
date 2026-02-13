#pragma once

#include <concepts>
#include <string_view>
#include <vector>

#include "flecs.h"

namespace atmo::core::ecs::entities
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(const flecs::entity &handle) : p_handle(handle) {}

        operator flecs::entity() const
        {
            return p_handle;
        }

        static constexpr std::string_view FullName()
        {
            return "Entity";
        }

        template <typename Component> void setComponent(Component &&component)
        {
            p_handle.set(std::forward<Component>(component));
        }

        static void RegisterSystems(flecs::world *world);

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

    protected:
        flecs::entity p_handle;
    };
} // namespace atmo::core::ecs::entities
