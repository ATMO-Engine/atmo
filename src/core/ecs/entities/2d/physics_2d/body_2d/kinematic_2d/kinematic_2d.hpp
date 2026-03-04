#pragma once

#include "core/ecs/entities/2d/physics_2d/body_2d/body_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/subresources/2d/shape/shape2d.hpp"

namespace atmo::core::ecs::entities
{
    class Kinematic2d : public EntityRegistry::Registrable<Kinematic2d, Body2d>
    {
    public:
        using EntityRegistry::Registrable<Kinematic2d, Body2d>::Registrable;

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Kinematic2d";
        }

        void setBodyType() override;

    private:
        struct Kinematic2dData {
        };
    };
} // namespace atmo::core::ecs::entities
