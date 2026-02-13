#pragma once

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    class Window : public EntityRegistry::Registrable<Window, Entity>
    {
    public:
        using EntityRegistry::Registrable<Window, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Window";
        }
    };
} // namespace atmo::core::ecs::entities
