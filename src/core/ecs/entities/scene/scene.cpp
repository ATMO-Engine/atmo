#include "scene.hpp"

#include "box2d/box2d.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/body_2d.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/entities/scene/physics_tasks.hpp"
#include "core/ecs/world_context.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/physics_progress_tick_event/physics_progress_tick_event.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "project/project_manager.hpp"
#include "server/rendering/rendering_server.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    b2DebugDraw Scene::m_debug_draw{ b2DefaultDebugDraw() };
    static constexpr int debug_draw_z_index = 1 << 20;

    void Scene::RegisterSystems(flecs::world *world)
    {
        SetupDebugDraw(&m_debug_draw);

        world->component<components::DebugDraw2d>();

        world->observer<components::Scene>("Scene_Destroy2dPhysicsWorld").event(flecs::OnRemove).each([](flecs::entity e, components::Scene &scene) {
            if (b2World_IsValid(scene.world_id)) {
                b2DestroyWorld(scene.world_id);
            }
        });

        world->observer<components::DebugDraw2d>("Scene_FreeDebugDraw2d").event(flecs::OnRemove).each([](flecs::entity, components::DebugDraw2d &debug) {
            server::rendering::RenderingServer::Instance().canvasItemFree(debug.item);
            debug.item = {};
        });

        const components::WorldContext *ctx = world->try_get<components::WorldContext>();
        const bool editor_world = ctx && ctx->is_editor_isolated;
        const auto &debug_settings = atmo::project::ProjectManager::GetSettings().debug;

        if (editor_world ? debug_settings.draw_physics_debug : debug_settings.draw_physics_debug_in_game) {
            world->system<components::Scene, components::DebugDraw2d>("Scene_DebugDraw2d")
                .kind(flecs::OnValidate)
                .each([](flecs::entity e, components::Scene &scene, components::DebugDraw2d &debug) {
                    const auto *ctx = e.world().try_get<components::WorldContext>();
                    if (!ctx || !ctx->canvas.isValid() || !b2World_IsValid(scene.world_id))
                        return;

                    auto &server = server::rendering::RenderingServer::Instance();
                    if (!debug.item.isValid()) {
                        debug.item = server.canvasItemCreate(ctx->canvas);
                        server.canvasItemSetZIndex(debug.item, debug_draw_z_index);
                    }

                    server.canvasItemClear(debug.item);
                    b2DebugDraw draw = m_debug_draw;
                    draw.context = &debug.item;
                    b2World_Draw(scene.world_id, &draw);
                });
        }

        if (editor_world) {
            spdlog::debug("Scene::RegisterSystems: skipping physics simulation for editor-isolated world");
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

                    b2BodyEvents events = b2World_GetBodyEvents(scene.world_id);

                    for (int i = 0; i < events.moveCount; ++i) {
                        const b2BodyMoveEvent *event = events.moveEvents + i;
                        Entity body(flecs::entity(it.world(), (flecs::entity_t)event->userData));
                        auto &transform = body.getComponentMutable<components::Transform2d>();
                        transform.position = event->transform.p;
                        transform.rotation = atmo::common::math::RadiansToDegrees(b2Rot_GetAngle(event->transform.q));
                    }

                    auto evt = event::EventRegistry::Create<event::events::PhysicsProgressTickEvent>("Event::PhysicsProgressTickEvent");
                    evt->delta_time = physics_dt;
                    event::EventRegistry::Dispatch(evt);
                }
            });
    }

    void Scene::initialize()
    {
        Entity::initialize();

        setComponent<components::Scene>({});
        setComponent<components::DebugDraw2d>({});
        auto scene = p_handle.get_ref<components::Scene>();

        b2WorldDef worldDef = b2DefaultWorldDef();

        auto gravity = atmo::project::ProjectManager::GetSettings().engine.gravity;
        worldDef.gravity = { .x = gravity.x, .y = gravity.y };
        worldDef.workerCount = physics::WorkerCount();
        worldDef.enqueueTask = physics::EnqueueTask;
        worldDef.finishTask = physics::FinishTask;

        scene->world_id = b2CreateWorld(&worldDef);
    }

    void Scene::loadFromData(const EntityData &data)
    {
        for (auto &child : getChildren()) child.destroy();

        const auto saved_world_id = getWorldId();
        flecs::world world = p_handle.world();
        deserializeInWorld(data, &world);

        auto scene_comp = p_handle.get_ref<components::Scene>();
        if (scene_comp)
            scene_comp->world_id = saved_world_id;
    }

    void Scene::setSingleton(bool singleton)
    {
        auto scene = p_handle.get_ref<components::Scene>();
        scene->singleton = singleton;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Scene);

ATMO_REGISTER_COMPONENT(atmo::core::components::Scene)
