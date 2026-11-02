#pragma once

#include "box2d/box2d.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"
#include "server/rendering/rid.hpp"

namespace atmo::core::components
{
    struct Scene {
        bool singleton{ false };
        b2WorldId world_id{ b2_nullWorldId };
    };

    struct DebugDraw2d {
        server::rendering::CanvasItemRid item;
    };
} // namespace atmo::core::components

template <> struct atmo::meta::ComponentMeta<atmo::core::components::Scene> {
    static constexpr const char *name = "Scene";
    static constexpr auto fields = std::make_tuple(atmo::meta::field<&atmo::core::components::Scene::singleton>("singleton"));
};

namespace atmo::core::ecs::entities
{
    class Scene : public EntityRegistry::Registrable<Scene, Entity>
    {
    public:
        using EntityRegistry::Registrable<Scene, Entity>::Registrable;

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Scene";
        }

        /**
         * @brief Replace this scene's content with @p data (a scene file parsed by PackedScene), creating the
         *        children in this scene's world. Destroys any existing children first. Preserves this
         *        scene's existing Box2D world_id (created in initialize()) across the load.
         *
         * @param data Parsed scene tree.
         */
        void loadFromData(const EntityData &data);

        void setSingleton(bool singleton);

        b2WorldId getWorldId() const
        {
            auto scene = p_handle.get_ref<components::Scene>();
            return scene->world_id;
        }

    private:
        static void SetupDebugDraw(b2DebugDraw *debugDraw);

        static b2DebugDraw m_debug_draw;
    };
} // namespace atmo::core::ecs::entities
