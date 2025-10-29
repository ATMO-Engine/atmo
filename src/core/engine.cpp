#include "engine.hpp"
#include "impl/window.hpp"

void atmo::core::Engine::stop()
{
    m_ecs.stop();
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
