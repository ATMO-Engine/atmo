#include "engine.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/dynamic_2d/dynamic_2d.hpp"
#include "core/ecs/entities/2d/physics_2d/body_2d/static_2d/static_2d.hpp"
#include "core/ecs/entities/2d/sprite_2d/sprite_2d.hpp"
#include "core/ecs/entities/window/window.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/resource/subresources/2d/shape/circle_shape2d.hpp"
#include "core/resource/subresources/2d/shape/rectangle_shape2d.hpp"
#include "core/types.hpp"
#include "ecs/entities/script/script.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"
#include "resource/subresource_registry.hpp"

#include "luau/luau.hpp"

void atmo::core::Engine::start()
{
    m_running.store(true);
    atmo::luau::Luau vm;

    auto window = ecs::EntityRegistry::Create<ecs::entities::Window>("Entity::Window");
    window->rename("_Root");
    window->setName(project::ProjectManager::GetSettings().app.project_name);
    window->onClose([&]() { stop(); });

    auto scene = ecs::EntityRegistry::Create<ecs::entities::Scene>("Entity::Scene");
    scene->setSingleton(false);

    auto script = ecs::EntityRegistry::Create<ecs::entities::Script>("Entity::Script");
    atmo::luau::ScriptInstance inst = vm.generateInstance();
    script->setScriptInstance(&inst);
    script->setScriptPath("project://assets/script/luau_main.luau");
    script->setParent(*scene);

    m_ecs.changeScene(scene);
    {
        auto rectangle_shape = resource::SubResourceRegistry::Create<resource::resources::RectangleShape2d>("SubResource::Shape2d::RectangleShape2d");
        rectangle_shape->setSize({ 800, 100 });

        auto static_body = ecs::EntityRegistry::Create<ecs::entities::Static2d>("Entity::Entity2d::Body2d::Static2d");
        static_body->addShape(rectangle_shape);
        static_body->setPosition({ 800, 500 });
        static_body->setParent(*scene);

        auto rectangle_shape2 = resource::SubResourceRegistry::Create<resource::resources::RectangleShape2d>("SubResource::Shape2d::RectangleShape2d");
        rectangle_shape2->setSize({ 80, 80 });

        auto dynamic_body = ecs::EntityRegistry::Create<ecs::entities::Dynamic2d>("Entity::Entity2d::Body2d::Dynamic2d");
        dynamic_body->addShape(rectangle_shape2);
        dynamic_body->setPosition({ 410, 300 });
        dynamic_body->setParent(*scene);

        auto circle_shape = resource::SubResourceRegistry::Create<resource::resources::CircleShape2d>("SubResource::Shape2d::CircleShape2d");
        circle_shape->setRadius(40.0f);
        circle_shape->getShapeDef().density = 2.0f;
        circle_shape->getShapeDef().material.rollingResistance = 0.02f;

        auto dynamic_body2 = ecs::EntityRegistry::Create<ecs::entities::Dynamic2d>("Entity::Entity2d::Body2d::Dynamic2d");
        dynamic_body2->addShape(circle_shape);
        dynamic_body2->setPosition({ 450, 0 });
        dynamic_body2->setParent(*scene);
    }

    auto last_time = std::chrono::steady_clock::now();
    float deltaTime = 0.0f;

    while (m_ecs.progress(deltaTime)) {
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = current_time - last_time;
        last_time = current_time;
        deltaTime = dt.count();

        if (atmo::core::InputManager::IsPressed("ui_quit"))
            m_running.store(false);

        atmo::core::InputManager::Tick();

        if (!m_running.load())
            m_ecs.stop();
    }
}

void atmo::core::Engine::stop()
{
    m_running.store(false);
}

void atmo::core::Engine::reset()
{
    m_ecs.reset();
}

atmo::core::ecs::ECS &atmo::core::Engine::getECS()
{
    return m_ecs;
}

// void atmo::core::Engine::init_systems()
// {
// ecs.system<components::Window>("PollEvents")
//     .kind(flecs::PreUpdate)
//     .each([this](flecs::iter &it, size_t, components::Window &window) {
//         impl::WindowManager *wm = static_cast<impl::WindowManager *>(component_managers[it.entity(0).id()]);
//         wm->pollEvents(it.delta_time());
//     });

// ecs.system<components::Window>("Draw")
//     .kind(flecs::PostUpdate)
//     .each([this](flecs::iter &it, size_t, components::Window &window) {
//         impl::WindowManager *wm = static_cast<impl::WindowManager *>(component_managers[it.entity(0).id()]);
//         wm->draw();
//     });
// ecs.observer<entities::Script>()
//     .event(flecs::OnAdd)
//     .each(
//         [](flecs::entity e, entities::Script &s)
//         {
//             lua_getglobal(s.luau.get_state(), "ready");

//             if (!lua_isfunction(s.luau.get_state(), -1)) {
//                 lua_pop(s.luau.get_state(), 1);
//                 return;
//             }

//             if (lua_pcall(s.luau.get_state(), 0, 0, 0) != LUA_OK) {
//                 spdlog::error("Error calling ready: {}", lua_tostring(s.luau.get_state(), -1));
//                 lua_pop(s.luau.get_state(), 1);
//             }

//             lua_pop(s.luau.get_state(), 1);
//         });

// ecs.system<entities::Script>("ScriptProcess")
//     .kind(flecs::OnUpdate)
//     .each(
//         [](flecs::iter &it, size_t, entities::Script &s)
//         {
//             lua_getglobal(s.luau.get_state(), "process");

//             if (!lua_isfunction(s.luau.get_state(), -1)) {
//                 lua_pop(s.luau.get_state(), 1);
//                 return;
//             }

//             lua_pushnumber(s.luau.get_state(), it.delta_time());

//             if (lua_pcall(s.luau.get_state(), 1, 0, 0) != LUA_OK) {
//                 spdlog::error("Error calling process: {}", lua_tostring(s.luau.get_state(), -1));
//                 lua_pop(s.luau.get_state(), 1);
//             }

//             lua_pop(s.luau.get_state(), 1);
//         });

// ecs.system<entities::Script>("ScriptPhysicsProcess")
//     .kind(flecs::PostUpdate)
//     .each(
//         [](flecs::iter &it, size_t, entities::Script &s)
//         {
//             lua_getglobal(s.luau.get_state(), "physics_process");

//             if (!lua_isfunction(s.luau.get_state(), -1)) {
//                 lua_pop(s.luau.get_state(), 1);
//                 return;
//             }

//             lua_pushnumber(s.luau.get_state(), it.delta_time());

//             if (lua_pcall(s.luau.get_state(), 1, 0, 0) != LUA_OK) {
//                 spdlog::error("Error calling physics_process: {}", lua_tostring(s.luau.get_state(), -1));
//                 lua_pop(s.luau.get_state(), 1);
//             }

//             lua_pop(s.luau.get_state(), 1);
//         });
// }
