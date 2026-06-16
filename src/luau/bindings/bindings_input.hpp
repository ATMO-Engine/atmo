#pragma once
#include "lualib.h"

namespace atmo::luau
{
    class InputBindings
    {
    public:
        static void RegisterType(lua_State *state)
        {
            lua_newtable(state);

            lua_pushcfunction(state, IsPressed, "Input.isPressed");
            lua_setfield(state, -2, "isPressed");

            lua_pushcfunction(state, IsJustPressed, "Input.isJustPressed");
            lua_setfield(state, -2, "isJustPressed");

            lua_pushcfunction(state, IsJustReleased, "Input.isJustReleased");
            lua_setfield(state, -2, "isJustReleased");

            lua_pushcfunction(state, IsReleased, "Input.isReleased");
            lua_setfield(state, -2, "isReleased");

            lua_pushcfunction(state, GetMousePosition, "Input.getMousePosition");
            lua_setfield(state, -2, "getMousePosition");

            lua_setglobal(state, "Input");
        }

    private:
        static int IsPressed(lua_State *state);
        static int IsJustPressed(lua_State *state);
        static int IsJustReleased(lua_State *state);
        static int IsReleased(lua_State *state);
        static int GetMousePosition(lua_State *state);
    };
} // namespace atmo::luau
