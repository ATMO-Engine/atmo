#include "scene.hpp"
#include "box2d/box2d.h"
#include "core/ecs/components.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/types.hpp"
#include "project/project_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo::core::ecs::entities
{
    b2DebugDraw Scene::m_debug_draw{ b2DefaultDebugDraw() };

    void Scene::RegisterComponents(flecs::world *world)
    {
        world->component<components::Scene>();

        m_debug_draw.drawShapes = true;
        m_debug_draw.useDrawingBounds = false;

        m_debug_draw.DrawSegmentFcn = [](b2Vec2 p1, b2Vec2 p2, b2HexColor color, void *context) {
            auto *window = static_cast<components::Window *>(context);

            types::ColorRGBAi rgba = common::Utils::HexToRGBAi(color);
            rgba.a = 255;

            types::Vector2 vec1 = p1;
            types::Vector2 vec2 = p2;

            SDL_SetRenderDrawColor(window->renderer_data.renderer, rgba.r, rgba.g, rgba.b, rgba.a);
            SDL_RenderLine(window->renderer_data.renderer, vec1.x, vec1.y, vec2.x, vec2.y);
        };

        m_debug_draw.DrawPolygonFcn = [](const b2Vec2 *vertices, int vertexCount, b2HexColor color, void *context) {
            auto *window = static_cast<components::Window *>(context);

            std::vector<SDL_FPoint> points(vertexCount);
            for (int i = 0; i < vertexCount; ++i) {
                types::Vector2 vec = vertices[i];
                points[i] = { vec.x, vec.y };
            }

            types::ColorRGBAi rgba = common::Utils::HexToRGBAi(color);
            rgba.a = 255;

            SDL_SetRenderDrawColor(window->renderer_data.renderer, rgba.r, rgba.g, rgba.b, rgba.a);
            SDL_RenderLines(window->renderer_data.renderer, points.data(), vertexCount);
        };

        m_debug_draw.DrawSolidPolygonFcn = [](b2Transform transform, const b2Vec2 *vertices, int vertexCount, float radius, b2HexColor color, void *context) {
            auto *window = static_cast<components::Window *>(context);

            std::vector<SDL_Vertex> points(vertexCount);

            types::ColorRGBA rgba{ common::Utils::HexToRGBAi(color) };
            rgba.a = 0.5f;

            for (int i = 0; i < vertexCount; ++i) {
                types::Vector2 vec = vertices[i];
                points[i] = SDL_Vertex{ .position = { vec.x, vec.y }, .color = rgba.toSDLColor(), .tex_coord = { 0, 0 } };
            }

            SDL_RenderGeometry(window->renderer_data.renderer, nullptr, points.data(), vertexCount, nullptr, 0);
        };
    }

    void Scene::RegisterSystems(flecs::world *world)
    {
        world->observer<components::Scene>("Scene_Destroy2dPhysicsWorld").event(flecs::OnRemove).each([](flecs::entity e, components::Scene &scene) {
            if (b2World_IsValid(scene.world_id)) {
                b2DestroyWorld(scene.world_id);
            }
        });

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

        // TODO: implement debug drawing with b2DebugDraw and call b2World_Draw in a debug system
        // b2World_Draw(b2WorldId worldId, b2DebugDraw *draw)

        scene->world_id = b2CreateWorld(&worldDef);
    }

    void Scene::initFromFile(std::string_view file_path) {}

    void Scene::setSingleton(bool singleton)
    {
        auto scene = p_handle.get_ref<components::Scene>();
        scene->singleton = singleton;
    }
} // namespace atmo::core::ecs::entities

REGISTER_ENTITY(entities::Scene);
