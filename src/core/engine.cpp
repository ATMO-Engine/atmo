#include "engine.hpp"

const std::map<std::string, flecs::entity> atmo::core::Engine::prefabs = {
    {"Script", ECS.prefab("Script").set(entities::Script{""})},
    // {"UIRectangle", ecs.prefab("UI Rectangle").set()}
};

void atmo::core::Engine::load_ecs()
{
    ecs.observer<entities::Script>()
        .event(flecs::OnAdd)
        .each(
            [](flecs::entity e, entities::Script &s)
            {
                lua_getglobal(s.luau.get_state(), "ready");

                if (!lua_isfunction(s.luau.get_state(), -1)) {
                    lua_pop(s.luau.get_state(), 1);
                    return;
                }

                if (lua_pcall(s.luau.get_state(), 0, 0, 0) != LUA_OK) {
                    spdlog::error("Error calling ready: {}", lua_tostring(s.luau.get_state(), -1));
                    lua_pop(s.luau.get_state(), 1);
                }

                lua_pop(s.luau.get_state(), 1);
            });

    ecs.system<entities::Script>("ScriptProcess")
        .kind(flecs::OnUpdate)
        .each(
            [](flecs::iter &it, size_t, entities::Script &s)
            {
                lua_getglobal(s.luau.get_state(), "process");

                if (!lua_isfunction(s.luau.get_state(), -1)) {
                    lua_pop(s.luau.get_state(), 1);
                    return;
                }

                lua_pushnumber(s.luau.get_state(), it.delta_time());

                if (lua_pcall(s.luau.get_state(), 1, 0, 0) != LUA_OK) {
                    spdlog::error("Error calling process: {}", lua_tostring(s.luau.get_state(), -1));
                    lua_pop(s.luau.get_state(), 1);
                }

                lua_pop(s.luau.get_state(), 1);
            });

    ecs.system<entities::Script>("ScriptPhysicsProcess")
        .kind(flecs::PostUpdate)
        .each(
            [](flecs::iter &it, size_t, entities::Script &s)
            {
                lua_getglobal(s.luau.get_state(), "physics_process");

                if (!lua_isfunction(s.luau.get_state(), -1)) {
                    lua_pop(s.luau.get_state(), 1);
                    return;
                }

                lua_pushnumber(s.luau.get_state(), it.delta_time());

                if (lua_pcall(s.luau.get_state(), 1, 0, 0) != LUA_OK) {
                    spdlog::error("Error calling physics_process: {}", lua_tostring(s.luau.get_state(), -1));
                    lua_pop(s.luau.get_state(), 1);
                }

                lua_pop(s.luau.get_state(), 1);
            });
}
