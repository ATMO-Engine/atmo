#include "scene.hpp"

#include "box2d/box2d.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/2d/camera_2d/camera_2d.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/body_2d.hpp"
#include "core/ecs/entities/entity.hpp"
#include "core/ecs/world_context.hpp"
#include "core/event/event_registry.hpp"
#include "core/event/events/physics_progress_tick_event/physics_progress_tick_event.hpp"
#include "core/thread/thread_pool.hpp"
#include "core/types.hpp"
#include "meta/auto_register.hpp"
#include "project/file.hpp"
#include "project/file_system.hpp"
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

        if (atmo::project::ProjectManager::GetSettings().debug.draw_physics_debug) {
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

        const components::WorldContext *ctx = world->try_get<components::WorldContext>();
        if (ctx && ctx->is_editor_isolated) {
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

    void *enqueueTask(b2TaskCallback *task, int itemCount, int minRange, void *taskContext, void *userContext)
    {
        atmo::core::TaskHandleGroup *group = new atmo::core::TaskHandleGroup();

        int maxTasks = atmo::core::ThreadPool::Instance().getWorkerCount() * 2;

        int taskCount = std::min(maxTasks, (itemCount + minRange - 1) / minRange);

        int chunkSize = (itemCount + taskCount - 1) / taskCount;

        for (int i = 0; i < taskCount; i++) {
            int start = i * chunkSize;
            int end = std::min(start + chunkSize, itemCount);

            if (start < end)
                group->tasks.emplace_back(atmo::core::ThreadPool::Instance().submit([=] { task(start, end, i, taskContext); }));
        }

        return group;
    }

    void finishTask(void *userTask, void *userContext)
    {
        for (auto task : ((atmo::core::TaskHandleGroup *)userTask)->tasks) task.wait();
        delete (atmo::core::TaskHandleGroup *)userTask;
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
        worldDef.workerCount = atmo::core::ThreadPool::Instance().getWorkerCount();
        worldDef.enqueueTask = enqueueTask;
        worldDef.finishTask = finishTask;

        scene->world_id = b2CreateWorld(&worldDef);
    }

    bool Scene::loadFromJson(const std::string &json, flecs::world *world)
    {
        for (auto &child : getChildren()) child.destroy();

        EntityData data;
        if (auto err = glz::read_json(data, json); err) {
            spdlog::error("Scene::loadFromJson: JSON parse error");
            return false;
        }

        const auto saved_world_id = getWorldId();
        deserializeInWorld(data, world);

        auto scene_comp = p_handle.get_ref<components::Scene>();
        if (scene_comp)
            scene_comp->world_id = saved_world_id;

        return true;
    }

    void Scene::initFromFile(std::string_view file_path)
    {
        try {
            project::File file = project::FileSystem::OpenFile(file_path);
            flecs::world world = p_handle.world();
            if (!loadFromJson(file.readAll(), &world))
                spdlog::error("Scene::initFromFile: failed to load '{}'", file_path);
        } catch (const std::exception &e) {
            spdlog::error("Scene::initFromFile: could not open '{}': {}", file_path, e.what());
        }
    }

    void Scene::setSingleton(bool singleton)
    {
        auto scene = p_handle.get_ref<components::Scene>();
        scene->singleton = singleton;
    }
} // namespace atmo::core::ecs::entities

ATMO_REGISTER_ENTITY(entities::Scene);

ATMO_REGISTER_COMPONENT(atmo::core::components::Scene)
