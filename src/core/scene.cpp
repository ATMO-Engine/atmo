#include "core/scene.hpp"
#include <utility>
#include "spdlog/spdlog.h"

Scene::Scene()
{
    prefabs = {
        {"Script", ecs.prefab("Script").set(Script{""})},
        // {"UIRectangle", ecs.prefab("UI Rectangle").set()}
    };

    // Script
    {
        ecs.observer<Script>()
            .event(flecs::OnAdd)
            .each(
                [](flecs::entity e, Script &s)
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

        ecs.system<const Script>("ScriptProcess")
            .kind(flecs::OnUpdate)
            .each(
                [](flecs::iter &it, const Script &s)
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

        ecs.system<const Script>("ScriptPhysicsProcess")
            .kind(flecs::PostUpdate)
            .each(
                [](flecs::iter &it, const Script &s)
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
}

void Scene::tick() { ecs.progress(); }
