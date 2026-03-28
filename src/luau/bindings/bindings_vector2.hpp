#pragma once

#include "lualib.h"

#include <memory>
#include "core/types.hpp"
#include "lua_bindings.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    template <> class LuaBindings<Vector2> : public atmo::luau::LuaBindingsBase<atmo::luau::LuaBindings<Vector2>, Vector2>
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

        static Property m_properties[];
        static constexpr const char *name = "Vector2";

    private:
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
    };
} // namespace atmo::luau
