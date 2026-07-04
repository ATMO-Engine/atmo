#pragma once

#include "SDL3/SDL_render.h"
#include "box2d/box2d.h"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/types.hpp"
#include "meta/meta.hpp"

// Context passed to editor-specific Box2D debug-draw callbacks.
// Carries the renderer and the camera transform (zoom + screen-space world origin).
struct EditorDebugContext {
    SDL_Renderer *renderer = nullptr;
    float zoom = 1.0f;
    atmo::core::types::Vector2 pan{ 0.0f, 0.0f };
};


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

        static void RegisterSystems(flecs::world *world);

        void initialize();

        static constexpr std::string_view LocalName()
        {
            return "Scene";
        }

        void initFromFile(std::string_view file_path);

        /**
         * @brief Parses @p json as an EntityData tree and deserializes it into this scene's children,
         *        creating them inside @p world. Destroys any existing children first. Preserves this
         *        scene's existing Box2D world_id (created in initialize()) across the load, since
         *        deserialized JSON may carry a stale/foreign world_id value.
         * @return true on success, false on JSON parse error.
         */
        bool loadFromJson(const std::string &json, flecs::world *world);

        void setSingleton(bool singleton);

        b2WorldId getWorldId() const
        {
            auto scene = p_handle.get_ref<components::Scene>();
            return scene->world_id;
        }

    private:
        static void SetupDebugDraw(b2DebugDraw *debugDraw);
        static void SetupEditorDebugDraw(b2DebugDraw *debugDraw);

        static b2DebugDraw m_debug_draw;
        static b2DebugDraw m_editor_debug_draw;
    };
} // namespace atmo::core::ecs::entities
