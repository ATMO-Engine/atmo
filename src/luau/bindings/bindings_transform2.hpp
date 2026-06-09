#pragma once

#include <memory>
#include "bindings_vector2.hpp"
#include "core/ecs/entities/2d/entity_2d.hpp"
#include "core/types.hpp"
#include "lua_bindings.hpp"
#include "lualib.h"


#include "core/ecs/entities/2d/entity_2d.hpp"


#include <iostream>


namespace atmo::luau
{
    using namespace atmo::core::components;

    template <> class LuaBindings<Transform2d> : public LuaBindingsBase<LuaBindings<Transform2d>, Transform2d>
    {
    public:
        static void RegisterType(lua_State *state)
        {
            std::cout << "register Type" << name << std::endl;

            luaL_newmetatable(state, name);

            //            lua_pushcfunction(state, GC, "Transform2d.__gc");
            //            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Index, "Transform2d.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, NewIndex, "Transform2d.__newindex");
            lua_setfield(state, -2, "__newindex");

            // No constructor exposed — Transform2d is obtained via getTransform()
            lua_pop(state, 1);
        }

        static Property m_properties[];
        static constexpr const char *name = "Transform2d";

        // Appelé depuis getTransform() pour push le composant sur la stack
        static void push(lua_State *L, Transform2d *t)
        {
            Transform2d **ud = (Transform2d **)lua_newuserdata(L, sizeof(Transform2d *));
            *ud = t;
            luaL_getmetatable(L, name);
            lua_setmetatable(L, -2);
        }

    private:
        static int GC(lua_State *state)
        {
            auto *ptr = (std::shared_ptr<Transform2d> *)luaL_checkudata(state, 1, name);
            ptr->~shared_ptr<Transform2d>();
            return 0;
        }
    };
} // namespace atmo::luau
