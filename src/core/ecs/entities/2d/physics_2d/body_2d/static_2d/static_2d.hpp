#pragma once

#include "core/ecs/entities/2d/physics_2d/body_2d/body_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/subresources/2d/shape/shape2d.hpp"
#include "meta/meta.hpp"

namespace atmo::core::ecs::entities
{
    class Static2d : public EntityRegistry::Registrable<Static2d, Body2d>
    {
    public:
        using EntityRegistry::Registrable<Static2d, Body2d>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Static2d";
        }

        void setBodyType() override;

        struct Static2dData {
        };
    };
} // namespace atmo::core::ecs::entities

template <> struct atmo::meta::ComponentMeta<atmo::core::ecs::entities::Static2d::Static2dData> {
    static constexpr const char *name = "Static2dData";
    static constexpr auto fields = std::make_tuple();
};
