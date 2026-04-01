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
    struct Transform2d {
        types::Vector2 position{ 0.0f, 0.0f };
        types::Vector2 g_position{ 0.0f, 0.0f };

        float rotation{ 0.0f };
        float g_rotation{ 0.0f };

        types::Vector2 scale{ 1.0f, 1.0f };
        types::Vector2 g_scale{ 1.0f, 1.0f };
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Transform2d> {
    static constexpr const char *name = "Transform2d";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::components::Transform2d::position>("position"),
        atmo::meta::field<&atmo::core::components::Transform2d::rotation>("rotation"), atmo::meta::field<&atmo::core::components::Transform2d::scale>("scale"));
};

namespace atmo::core::ecs::entities
{
    class Entity2d : public EntityRegistry::Registrable<Entity2d, Entity>
    {
    public:
        using EntityRegistry::Registrable<Entity2d, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Entity2d";
        }

        types::Vector2 getPosition() const;
        void setPosition(const types::Vector2 &position);
        types::Vector2 getGlobalPosition() const;

        types::Vector2 getScale() const;
        void setScale(const types::Vector2 &scale);
        types::Vector2 getGlobalScale() const;

        float getRotation() const;
        void setRotation(float rotation);
        float getGlobalRotation() const;
    };
} // namespace atmo::core::ecs::entities
