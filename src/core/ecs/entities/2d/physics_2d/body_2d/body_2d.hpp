#pragma once

#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/subresources/2d/shape/shape2d.hpp"

namespace atmo::core::ecs::entities
{
    class Body2d : public EntityRegistry::Registrable<Body2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Body2d, Entity2d>::Registrable;

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Body2d";
        }

        void setPosition(const types::Vector2 &position);
        void setScale(const types::Vector2 &scale);
        void setRotation(float rotation);

    private:
        struct Body2dData {
            b2BodyId body_id{ b2_nullBodyId };
            b2BodyDef body_def{ b2DefaultBodyDef() };
            std::vector<std::shared_ptr<resource::resources::Shape2d>> shapes;
        };
    };
} // namespace atmo::core::ecs::entities
