#include "engine.hpp"
#include "core/ecs/entity_registry.hpp"
#include "core/input/input_manager.hpp"
#include "core/resource/subresource_registry.hpp"
#include "core/types.hpp"
#include "impl/window.hpp"
#include "project/file_system.hpp"
#include "project/project_manager.hpp"

void atmo::core::Engine::start()
{
    m_running.store(true);

    auto window = m_ecs.instantiatePrefab("window", "_Root");
    atmo::impl::WindowManager *wm = static_cast<atmo::impl::WindowManager *>(window.get<atmo::core::ComponentManager::Managed>().ptr);
    wm->rename(atmo::project::ProjectManager::GetSettings().app.project_name);

    auto scene = m_ecs.instantiatePrefab("scene");
    m_ecs.changeScene(scene);

    // auto ptr = resource::SubResourceRegistry::Create("Shape2d::RectangleShape2d");
    // if (!ptr)
    //     spdlog::error("Failed to create subresource");
    // else
    //     spdlog::info("Created subresource of type {}", ptr->name().data());

    // auto ground = m_ecs.instantiatePrefab("static_body_2d", "ground").child_of(scene);
    // ground.get_ref<components::PhysicsBody2d>()->shape = resource::resources::Shape2d::Shape2dType::Rectangle;
    // ground.modified<components::PhysicsBody2d>();

    // auto last_time = std::chrono::steady_clock::now();
    // float deltaTime = 0.0f;

    // while (m_ecs.progress(deltaTime)) {
    //     auto current_time = std::chrono::steady_clock::now();
    //     std::chrono::duration<float> dt = current_time - last_time;
    //     last_time = current_time;
    //     deltaTime = dt.count();

    //     if (atmo::core::InputManager::IsPressed("ui_quit")) {
    //         spdlog::info("Quitting...");
    //         m_running.store(false);
    //     }

    //     atmo::core::InputManager::Tick();

    //     if (!m_running.load()) {
    //         m_ecs.stop();
    //     }
    // }
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
