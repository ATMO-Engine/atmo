#pragma once

#include <memory>
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/scene.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "flecs.h"
#include "meta/meta.hpp"


namespace atmo::core::components
{
    struct Transform3d {
        types::Vector3 position{ 0.0f, 0.0f, 0.0f };
        types::Vector3 g_position{ 0.0f, 0.0f, 0.0f };

        float rotation{ 0.0f };
        float g_rotation{ 0.0f };

        types::Vector3 scale{ 1.0f, 1.0f, 1.0f };
        types::Vector3 g_scale{ 1.0f, 1.0f, 1.0f };
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Transform3d> {
    static constexpr const char *name = "Transform3d";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Transform3d::position>("position"),
        atmo::meta::field<&atmo::core::components::Transform3d::rotation>("rotation"), atmo::meta::field<&atmo::core::components::Transform3d::scale>("scale"));
};

namespace atmo::core::ecs::entities
{
    class Entity3d : public EntityRegistry::Registrable<Entity3d, Entity>
    {
    public:
        using EntityRegistry::Registrable<Entity3d, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Entity3d";
        }

        types::Vector3 getPosition() const;
        void setPosition(const types::Vector3 &position);
        types::Vector3 getGlobalPosition() const;

        types::Vector3 getScale() const;
        void setScale(const types::Vector3 &scale);
        types::Vector3 getGlobalScale() const;

        float getRotation() const;
        void setRotation(float rotation);
        float getGlobalRotation() const;

        SDL_FRect computeAABB() const override;
    };
} // namespace atmo::core::ecs::entities
