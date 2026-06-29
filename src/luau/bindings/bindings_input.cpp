#include "bindings_input.hpp"
#include "bindings_vector2.hpp"
#include "core/input/input_manager.hpp"

namespace atmo::luau
{
    int InputBindings::IsPressed(lua_State *state)
    {
        const char *name = luaL_checkstring(state, 1);
        lua_pushboolean(state, atmo::core::InputManager::IsPressed(name));
        return 1;
    }

    int InputBindings::IsJustPressed(lua_State *state)
    {
        const char *name = luaL_checkstring(state, 1);
        lua_pushboolean(state, atmo::core::InputManager::IsJustPressed(name));
        return 1;
    }

    int InputBindings::IsJustReleased(lua_State *state)
    {
        const char *name = luaL_checkstring(state, 1);
        lua_pushboolean(state, atmo::core::InputManager::IsJustReleased(name));
        return 1;
    }

    int InputBindings::IsReleased(lua_State *state)
    {
        const char *name = luaL_checkstring(state, 1);
        lua_pushboolean(state, atmo::core::InputManager::IsReleased(name));
        return 1;
    }

    int InputBindings::GetMousePosition(lua_State *state)
    {
        auto pos = atmo::core::InputManager::GetMousePosition();
        LuaBindings<atmo::core::types::Vector2>::push(state, new atmo::core::types::Vector2(pos), true);
        return 1;
    }
} // namespace atmo::luau
