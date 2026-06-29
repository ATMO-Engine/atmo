#include "scene.hpp"

#include "SDL3/SDL_render.h"
#include "box2d/box2d.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/world_context.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    b2DebugDraw Scene::m_debug_draw{ b2DefaultDebugDraw() };
    b2DebugDraw Scene::m_editor_debug_draw{ b2DefaultDebugDraw() };
    static EditorDebugContext s_editor_debug_ctx;


    void Scene::RegisterSystems(flecs::world *world)
    {
        SetupDebugDraw(&m_debug_draw);

        world->observer<components::Scene>("Scene_Destroy2dPhysicsWorld").event(flecs::OnRemove).each([](flecs::entity e, components::Scene &scene) {
            if (b2World_IsValid(scene.world_id)) {
                b2DestroyWorld(scene.world_id);
            }
        });

        const components::WorldContext *ctx = world->try_get<components::WorldContext>();
        const bool is_isolated = ctx && ctx->is_editor_isolated;

        if (is_isolated) {
            spdlog::debug("Scene::RegisterSystems: skipping physics simulation for editor-isolated world");

            if (atmo::project::ProjectManager::GetSettings().debug.draw_physics_debug) {
                SetupEditorDebugDraw(&m_editor_debug_draw);
                world->system<components::Scene>("Body2d_DebugDrawShapes_Editor").kind(flecs::OnValidate).each([](flecs::entity e, components::Scene &scene) {
                    const auto *wctx = e.world().try_get<components::WorldContext>();
                    const auto *cam = e.world().try_get<components::WorldCameraState>();
                    if (!wctx || !wctx->renderer)
                        return;

                    int vp_w = 0, vp_h = 0;
                    SDL_GetCurrentRenderOutputSize(wctx->renderer, &vp_w, &vp_h);

                    const float zoom = (cam && cam->has_camera) ? cam->zoom : 1.0f;
                    const float px = static_cast<float>(vp_w) * 0.5f - (cam && cam->has_camera ? cam->position.x * zoom : 0.0f);
                    const float py = static_cast<float>(vp_h) * 0.5f - (cam && cam->has_camera ? cam->position.y * zoom : 0.0f);

                    s_editor_debug_ctx = { wctx->renderer, zoom, { px, py } };
                    m_editor_debug_draw.context = &s_editor_debug_ctx;

                    if (b2World_IsValid(scene.world_id))
                        b2World_Draw(scene.world_id, &m_editor_debug_draw);
                });
            }
            return;
        }

        static const float physics_dt = 1.0f / atmo::project::ProjectManager::GetSettings().engine.physics_frame_rate;

        auto Physics = world->entity("Physics").add(flecs::Phase).depends_on(flecs::OnUpdate);
        flecs::entity physics_tick = world->timer().interval(physics_dt);
        world->system<components::Scene>("Scene_Update2dPhysics")
            .kind(Physics)
            .tick_source(physics_tick)
            .each([&](flecs::iter &it, size_t i, components::Scene &scene) {
                if (b2World_IsValid(scene.world_id)) {
                    b2World_Step(scene.world_id, physics_dt, 4);
                }
            });

        if (atmo::project::ProjectManager::GetSettings().debug.draw_physics_debug) {
            world->system<components::Scene, components::Window>("Body2d_DebugDrawShapes")
                .kind(flecs::OnValidate)
                .term_at(1)
                .up()
                .each([&](flecs::iter &it, size_t i, components::Scene &scene, components::Window &window) {
                    m_debug_draw.context = &window;

                    b2World_Draw(scene.world_id, &m_debug_draw);
                });
        }
    }

    void Scene::initialize()
    {
        Entity::initialize();

        setComponent<components::Scene>({});
        auto scene = p_handle.get_ref<components::Scene>();

        b2WorldDef worldDef = b2DefaultWorldDef();

        auto gravity = atmo::project::ProjectManager::GetSettings().engine.gravity;
        worldDef.gravity = { .x = gravity.x, .y = gravity.y };

        scene->world_id = b2CreateWorld(&worldDef);
    }

    void Scene::initFromFile(std::string_view file_path) {}

    void Scene::setSingleton(bool singleton)
    {
        auto scene = p_handle.get_ref<components::Scene>();
        scene->singleton = singleton;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Scene);

ATMO_REGISTER_COMPONENT(atmo::core::components::Scene)
