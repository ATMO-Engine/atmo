#pragma once

#include "SDL3/SDL_render.h"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/resource/subresources/2d/shape/shape2d.hpp"
#include "meta/meta.hpp"

namespace atmo::core::ecs::entities
{
    class Body2d : public EntityRegistry::Registrable<Body2d, Entity2d>
    {
    public:
        using EntityRegistry::Registrable<Body2d, Entity2d>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Body2d";
        }

        void setPosition(const types::Vector2 &position);
        void setScale(const types::Vector2 &scale);
        void setRotation(float rotation);
        void setParent(Entity parent);

        void addShape(std::shared_ptr<resource::resources::Shape2d> shape);

        struct Body2dData {
            b2BodyId body_id{ b2_nullBodyId };
            b2BodyDef body_def{ b2DefaultBodyDef() };
            std::vector<std::shared_ptr<resource::resources::Shape2d>> shapes;
        };

    protected:
        virtual void setBodyType() = 0;

    private:
        void initBody();

        static void DebugRenderRectangleShape(SDL_Renderer *renderer, types::Vector2 center, types::Vector2 size, float angle);
    };
} // namespace atmo::core::ecs::entities

template <> struct atmo::meta::ComponentMeta<atmo::core::ecs::entities::Body2d::Body2dData> {
    static constexpr const char *name = "Body2dData";
    static constexpr const char *category = "2D";
    static constexpr auto fields = std::make_tuple();
};
