#pragma once

#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entity_registry.hpp"

namespace atmo::core::ecs::entities
{
    class Entity2d : public EntityRegistry::Registrable<Entity2d, Entity>
    {
    public:
        using EntityRegistry::Registrable<Entity2d, Entity>::Registrable;

        void init();

        static constexpr std::string_view LocalName()
        {
            return "Entity2d";
        }
    };
} // namespace atmo::core::ecs::entities
