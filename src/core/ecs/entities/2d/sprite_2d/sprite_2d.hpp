#pragma once

#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    class Sprite2d : public EntityRegistry::Registrable<Sprite2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Sprite2d, Entity2d>::Registrable;

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Sprite2d";
        }
    };
} // namespace atmo::core::ecs::entities
