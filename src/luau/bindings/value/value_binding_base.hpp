#pragma once

#include <new>
#include <type_traits>

#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau {
    /**
     * @brief
     * Bindings for small value types (Vector2, Color...): the value is copied inside the userdata,
     * so Lua never holds a pointer to C++ memory. Reading a property of this type gives a copy,
     * modifying it doesn't modify the source (write the whole value back instead).
     */
    template <typename Derived, typename T>
    class ValueLuaBindings : public LuaBindingsBase<Derived, T>
    {
    public:
        /**
         * @brief Push a copy of value inside the luau context
         *
         * @param L the luau instance where the code runs
         * @param value the value to copy
         */
        static void Push(lua_State *L, const T &value)
        {
            void *ud = nullptr;
            if constexpr (std::is_trivially_destructible_v<T>)
                ud = lua_newuserdata(L, sizeof(T));
            else
                ud = lua_newuserdatadtor(L, sizeof(T), [](void *p) { static_cast<T *>(p)->~T(); });
            new (ud) T(value);

            luaL_getmetatable(L, Derived::name);
            lua_setmetatable(L, -2);
        }

        static T *CheckPtrImpl(lua_State *L, int index)
        {
            return static_cast<T *>(luaL_checkudata(L, index, Derived::name));
        }
    };
} // namespace atmo::luau
