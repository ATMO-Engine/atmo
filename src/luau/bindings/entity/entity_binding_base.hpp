#pragma once

#include <memory>
#include "core/ecs/entities/entity.hpp"
#include "lua.h"
#include "lualib.h"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau {
    template <typename Derived, typename T, typename ParentBinding = void>
    class EntityLuaBindings : public LuaBindingsBase<Derived, T, ParentBinding>
    {
    public:
        static void Push(lua_State *L, const T &entity)
        {
            auto *ud = static_cast<T *>(
                lua_newuserdatadtor(L, sizeof(T), [](void *p) {
                    static_cast<T *>(p)->~T();
                }));
            new (ud) T(entity);

            luaL_getmetatable(L, Derived::name);
            lua_setmetatable(L, -2);
        }

        /**
         * @brief Only checks the type, the entity may be destroyed:
         * the handle must be checked with is_alive() before use
         */
        static T* CheckPtrImpl(lua_State* L, int index)
        {
            if (!LuaBindingsBase<Derived, T, ParentBinding>::CheckIsA(L, index, Derived::name))
                luaL_typeerrorL(L, index, Derived::name);

            return static_cast<T *>(lua_touserdata(L, index));
        }
    };
} // namespace atmo::luau
