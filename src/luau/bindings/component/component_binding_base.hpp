#pragma once

#include <new>

#include <flecs.h>

#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau {
    /**
     * @brief
     * What a component userdata holds: never a raw pointer to flecs memory (it moves when the
     * entity changes), but the entity, the component is fetched again on every access.
     */
    struct ComponentHandle {
        void *owned = nullptr;
        flecs::entity entity;
    };

    template <typename Derived, typename T, typename ParentBinding = void>
    class ComponentLuaBindings : public LuaBindingsBase<Derived, T>
    {
    public:
        /**
         * @brief Push an object created for Lua, Lua takes ownership and deletes it when collected
         *
         * @param L the luau instance where the code runs
         * @param t the object, allocated with new
         */
        static void PushOwned(lua_State *L, T *t)
        {
            Create(L)->owned = t;
        }

        /**
         * @brief Push the T component of an entity, it is looked up on every access so the
         * userdata stays safe if the entity is destroyed or the component moved / removed
         *
         * @param L the luau instance where the code runs
         * @param entity the entity holding the component
         */
        static void PushComponent(lua_State *L, flecs::entity entity)
        {
            Create(L)->entity = entity;
        }

        static T *CheckPtrImpl(lua_State *L, int index)
        {
            auto *ud = static_cast<ComponentHandle *>(luaL_checkudata(L, index, Derived::name));

            if (ud->owned)
                return static_cast<T *>(ud->owned);

            if (!ud->entity.is_alive())
                luaL_error(L, "%s: the entity holding this component has been destroyed", Derived::name);

            T *component = ud->entity.template try_get_mut<T>();
            if (!component)
                luaL_error(L, "%s: the component has been removed from its entity", Derived::name);

            return component;
        }

    private:
        static ComponentHandle *Create(lua_State *L)
        {
            auto *ud = new (lua_newuserdatadtor(L, sizeof(ComponentHandle), Destroy)) ComponentHandle{};
            luaL_getmetatable(L, Derived::name);
            lua_setmetatable(L, -2);
            return ud;
        }

        /**
         * @brief
         * Called by Luau when the userdata is collected, delete the object if luau owns it
         *
         * @param p The ComponentHandle stored inside the userdata
         */
        static void Destroy(void *p)
        {
            auto *ud = static_cast<ComponentHandle *>(p);
            delete static_cast<T *>(ud->owned);
            ud->~ComponentHandle();
        }
    };
} // namespace atmo::luau
