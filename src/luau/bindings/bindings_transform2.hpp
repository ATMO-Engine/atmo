#pragma once

#include <memory>
#include "bindings_vector2.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/types.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"

namespace atmo::luau
{
    using namespace atmo::core::components;

    template <> class LuaBindings<Transform2d> : public LuaBindingsBase<LuaBindings<Transform2d>, Transform2d>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            luaL_newmetatable(state, name);

            lua_pushcfunction(state, GC, "Transform2d.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Transform2d.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Transform2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_pop(state, 1);

            lua_pushcfunction(state, New, name);
            lua_setglobal(state, name);
        }

        static Property m_properties[];
        static constexpr const char *name = "Transform2d";

        /**
         * @brief Push a Transform2d instance inside the luau context is it stored inside
         * a shared_ptr with a no-op deleter before being pushed to the luau
         *
         * @param L the luau instance where the code runs
         * @param t the Transform2d you want to push
         */
        static void push(lua_State *L, Transform2d *t)
        {
            void *mem = lua_newuserdata(L, sizeof(std::shared_ptr<Transform2d>));
            // No op deleter since luau doesn't hold the instance of the object
            new (mem) std::shared_ptr<Transform2d>(t, [](Transform2d *) {});
            luaL_getmetatable(L, name);
            lua_setmetatable(L, -2);
        }

        /**
         * @brief Push a Transform2d instance inside the luau context
         *
         * @param L the luau instance where the code runs
         * @param t the Transform2d you want to push that is already inside a shared_ptr
         */
        static void push(lua_State *L, std::shared_ptr<Transform2d> t)
        {
            void *mem = lua_newuserdata(L, sizeof(std::shared_ptr<Transform2d>));
            new (mem) std::shared_ptr<Transform2d>(std::move(t));
            luaL_getmetatable(L, name);
            lua_setmetatable(L, -2);
        }

    private:
        static int New(lua_State *state)
        {
            void *mem = lua_newuserdata(state, sizeof(std::shared_ptr<Transform2d>));
            new (mem) std::shared_ptr<Transform2d>(std::make_shared<Transform2d>());
            luaL_getmetatable(state, name);
            lua_setmetatable(state, -2);
            return 1;
        }

        static int GC(lua_State *state)
        {
            auto *ptr = (std::shared_ptr<Transform2d> *)luaL_checkudata(state, 1, name);
            ptr->~shared_ptr<Transform2d>();
            return 0;
        }
    };
} // namespace atmo::luau
