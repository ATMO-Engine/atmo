#pragma once

#include <memory>
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/types.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"

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

        /**
         * @brief Push a Vector2 instance inside the luau context is it stored inside
         * a shared_ptr with a no-op deleter before being pushed to the luau
         *
         * @param L the luau instance where the code runs
         * @param t the Vector2 you want to push
         */
        static void push(lua_State *L, Vector2 *v)
        {
            void *mem = lua_newuserdata(L, sizeof(std::shared_ptr<Vector2>));
            new (mem) std::shared_ptr<Vector2>(v, [](Vector2 *) {});
            luaL_getmetatable(L, name);
            lua_setmetatable(L, -2);
        }


        /**
         * @brief Push a Vector2 instance inside the luau context
         *
         * @param L the luau instance where the code runs
         * @param t the Vector2 you want to push that is already inside a shared_ptr
         */
        static void push(lua_State *L, std::shared_ptr<Vector2> v)
        {
            void *mem = lua_newuserdata(L, sizeof(std::shared_ptr<Vector2>));
            new (mem) std::shared_ptr<Vector2>(std::move(v));
            luaL_getmetatable(L, name);
            lua_setmetatable(L, -2);
        }

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
