#pragma once

#include <concepts>

#include "flecs.h"

namespace atmo::core::ecs::entities
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(const flecs::entity &handle) : p_handle(handle) {}
        virtual ~Entity() = default;
        operator flecs::entity() const
        {
            return p_handle;
        }

        template <typename Component> void setComponent(Component &&component)
        {
            p_handle.set(std::forward<Component>(component));
        }

        virtual void initComponents() = 0;

    protected:
        flecs::entity p_handle;
    };
} // namespace atmo::core::ecs::entities
