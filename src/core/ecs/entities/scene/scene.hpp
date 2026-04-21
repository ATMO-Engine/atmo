#pragma once

#include "box2d/box2d.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "meta/meta.hpp"


namespace atmo::core::components
{
    struct Scene {
        bool singleton{ false };
        b2WorldId world_id{ b2_nullWorldId };
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

        static void RegisterComponents(flecs::world *world);
        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Scene";
        }

        void initFromFile(std::string_view file_path);

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
