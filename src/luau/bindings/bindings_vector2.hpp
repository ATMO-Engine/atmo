#pragma once

#include "lualib.h"

#include <cstring>
#include <memory>
#include "core/types.hpp"
#include "lua_bindings.hpp"

namespace atmo::luau
{

    using namespace atmo::core::types;

    template <> class LuaBindings<Vector2>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "Vector2.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Vector2.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Vector2.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, Length, "Vector2.length");
            lua_setfield(state, -2, "length");

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);
        }

    private:
        static constexpr const char *name = "Vector2";

        static std::shared_ptr<Vector2> &Check(lua_State *state, int index)
        {
            return *(std::shared_ptr<Vector2> *)luaL_checkudata(state, index, name);
        }

        static int New(lua_State *state)
        {
            float x = (float)luaL_checknumber(state, 1);
            float y = (float)luaL_checknumber(state, 2);

            void *mem = lua_newuserdata(state, sizeof(std::shared_ptr<Vector2>));
            new (mem) std::shared_ptr<Vector2>(std::make_shared<Vector2>(x, y));

            luaL_getmetatable(state, name);
            lua_setmetatable(state, -2);

            return 1;
        }

        static int GC(lua_State *state)
        {
            auto *ptr = (std::shared_ptr<Vector2> *)luaL_checkudata(state, 1, name);
            ptr->~shared_ptr<Vector2>();
            return 0;
        }

        static int Length(lua_State *state)
        {
            auto &v = Check(state, 1);
            lua_pushnumber(state, v->length());
            return 1;
        }

        static int Index(lua_State *state)
        {
            auto &v = Check(state, 1);
            const char *key = luaL_checkstring(state, 2);

            if (strcmp(key, "x") == 0) {
                lua_pushnumber(state, v->x);
                return 1;
            }
            if (strcmp(key, "y") == 0) {
                lua_pushnumber(state, v->y);
                return 1;
            }

            luaL_getmetatable(state, name);
            lua_getfield(state, -1, "__methods");
            lua_getfield(state, -1, key);

            return 1;
        }

        static int NewIndex(lua_State *state)
        {
            auto &v = Check(state, 1);
            const char *key = luaL_checkstring(state, 2);
            float value = (float)luaL_checknumber(state, 3);

            if (strcmp(key, "x") == 0) {
                v->x = value;
                return 0;
            }
            if (strcmp(key, "y") == 0) {
                v->y = value;
                return 0;
            }

            return 0;
        }
    };
} // namespace atmo::luau
