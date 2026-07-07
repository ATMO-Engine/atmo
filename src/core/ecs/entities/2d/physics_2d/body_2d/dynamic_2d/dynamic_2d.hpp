#pragma once

#include "core/ecs/entities/2d/physics_2d/body_2d/body_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/subresources/2d/shape/shape2d.hpp"
#include "meta/meta.hpp"

namespace atmo::core::ecs::entities
{
    class Dynamic2d : public EntityRegistry::Registrable<Dynamic2d, Body2d>
    {
    public:
        using EntityRegistry::Registrable<Dynamic2d, Body2d>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Dynamic2d";
        }

        void setBodyType() override;
    };
} // namespace atmo::core::ecs::entities
