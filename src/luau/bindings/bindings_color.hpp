#pragma once

#include "lualib.h"

#include <cstring>
#include <memory>
#include "core/types.hpp"
#include "lua_bindings.hpp"

namespace atmo::luau
{

    using namespace atmo::core::types;

    template <> class LuaBindings<ColorRGBA>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "ColorRGBA.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "ColorRGBA.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "ColorRGBA.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);
        }

    private:
        static constexpr const char *name = "ColorRGBA";

        static std::shared_ptr<ColorRGBA> &Check(lua_State *state, int index)
        {
            return *(std::shared_ptr<ColorRGBA> *)luaL_checkudata(state, index, name);
        }

        static int New(lua_State *state)
        {
            float r = (float)luaL_checknumber(state, 1);
            float g = (float)luaL_checknumber(state, 2);
            float b = (float)luaL_checknumber(state, 3);
            float a = (float)luaL_checknumber(state, 4);

            void *mem = lua_newuserdata(state, sizeof(std::shared_ptr<ColorRGBA>));
            new (mem) std::shared_ptr<ColorRGBA>(std::make_shared<ColorRGBA>(r, g, b, a));

            luaL_getmetatable(state, name);
            lua_setmetatable(state, -2);

            return 1;
        }

        static int GC(lua_State *state)
        {
            auto *ptr = (std::shared_ptr<ColorRGBA> *)luaL_checkudata(state, 1, name);
            ptr->~shared_ptr<ColorRGBA>();
            return 0;
        }

        static int Index(lua_State *state)
        {
            auto &v = Check(state, 1);
            const char *key = luaL_checkstring(state, 2);

            if (strcmp(key, "r") == 0) {
                lua_pushnumber(state, v->r);
                return 1;
            }
            if (strcmp(key, "g") == 0) {
                lua_pushnumber(state, v->g);
                return 1;
            }
            if (strcmp(key, "b") == 0) {
                lua_pushnumber(state, v->b);
                return 1;
            }
            if (strcmp(key, "a") == 0) {
                lua_pushnumber(state, v->a);
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

            if (strcmp(key, "r") == 0) {
                v->r = value;
                return 0;
            }
            if (strcmp(key, "g") == 0) {
                v->g = value;
                return 0;
            }
            if (strcmp(key, "b") == 0) {
                v->b = value;
                return 0;
            }
            if (strcmp(key, "a") == 0) {
                v->a = value;
                return 0;
            }

            return 0;
        }
    };
} // namespace atmo::luau
