#pragma once

#include <functional>
#include <iostream>
#include "lua.h"
#include "lualib.h"

#include "core/ecs/entities/2d/entity_2d.hpp"

namespace atmo
{
    namespace luau
    {
        struct Property;

        struct ComponentHandle {
            void *component = nullptr;
            bool owned = false;
        };

        template <typename T> class LuaBindings
        {
        public:
            /**
             * @brief
             * Register C++ bindings
             * @param state The VM in which you want to register the bindings
             */
            static void RegisterType(lua_State *state)
            {
                // TODO: spdlog generate crash here
                return;
            }

            static Property m_properties[];
            static constexpr const char *name = "Unknown";
        };

        template <typename Derived, typename T> class LuaBindingsBase
        {
        public:
            /**
             * @brief Push a type T instance inside the luau context the luau doesn't claim ownership
             *  over the instance
             *
             * @param L the luau instance where the code runs
             * @param t the type T you want to push
             */
            static void push(lua_State *L, T *t)
            {
                auto *ud = static_cast<ComponentHandle *>(lua_newuserdata(L, sizeof(ComponentHandle)));
                ud->component = t;
                ud->owned = false;
                luaL_getmetatable(L, Derived::name);
                lua_setmetatable(L, -2);
            }

            /**
             * @brief Push a type T instance inside the luau context, the context can claim ownership
             * over the object if wanted
             *
             * @param L the luau instance where the code runs
             * @param t the type T you want to push that is already inside a shared_ptr
             * @param owned tell the luau if it should claim ownership
             */
            static void push(lua_State *L, T *t, bool owned)
            {
                auto *ud = static_cast<ComponentHandle *>(lua_newuserdata(L, sizeof(ComponentHandle)));
                ud->component = t;
                ud->owned = owned;
                luaL_getmetatable(L, Derived::name);
                lua_setmetatable(L, -2);
            }

            /**
             * @brief
             * Get a T* from the luau
             *
             * @param L The vm / execution context in which you perform the action
             * @param index the index in the stack for the T object
             * @return T* the object requested, always check the value returned
             */
            static T *check_ptr(lua_State *L, int index)
            {
                auto *ud = static_cast<ComponentHandle *>(luaL_checkudata(L, index, Derived::name));
                if (!ud->component)
                    luaL_error(L, "%s: accessing destroyed component", Derived::name);
                return static_cast<T *>(ud->component);
            }

        protected:
            /**
             * @brief
             * Delete an oobject from the luau
             *
             * @param L The vm / execution context in which you perform the action
             * @return int Number of value returned in stack
             */
            static int GC(lua_State *L)
            {
                auto *ud = static_cast<ComponentHandle *>(luaL_checkudata(L, 1, Derived::name));
                if (ud->owned)
                    delete static_cast<T *>(ud->component);
                ud->component = nullptr;
                return 0;
            }

            /**
             * @brief
             * Push value read to the lua stack in order to get a element
             *
             * @param L The vm / execution context in which you perform the action
             * @return int Number of value returned in stack
             */
            static int Index(lua_State *L)
            {
                void *obj = luaL_checkudata(L, 1, Derived::name);
                const char *key = luaL_checkstring(L, 2);

                auto props = Derived::m_properties;

                for (int i = 0; props[i].name; i++) {
                    if (strcmp(props[i].name, key) == 0) {
                        props[i].getter(L, obj);
                        return 1;
                    }
                }

                luaL_getmetatable(L, Derived::name);
                lua_getfield(L, -1, "__methods");
                lua_getfield(L, -1, key);

                return 1;
            }

            /**
             * @brief
             * Set value of a element
             *
             * @param L The vm / execution context in which you perform the action
             * @return int Number of value returned in stack
             */
            static int NewIndex(lua_State *L)
            {
                void *obj = luaL_checkudata(L, 1, Derived::name);
                const char *key = luaL_checkstring(L, 2);

                auto props = Derived::m_properties;

                for (int i = 0; props[i].name; i++) {
                    if (strcmp(props[i].name, key) == 0) {
                        props[i].setter(L, obj);
                        return 0;
                    }
                }

                return 0;
            }
        };


        struct Property {
            const char *name;

            std::function<void(lua_State *, void *)> getter;
            std::function<void(lua_State *, void *)> setter;
        };

        template <typename T> void push_value(lua_State *L, T value)
        {
            if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                lua_pushnumber(L, value);
            } else if constexpr (std::is_integral_v<T>) {
                lua_pushinteger(L, value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                lua_pushstring(L, value.c_str());
            } else if constexpr (std::is_same_v<T, const char *>) {
                lua_pushstring(L, value);
            } else {
                static_assert(sizeof(T) == 0, "Unsupported type in push_value");
            }
        }

        template <typename T> T read_value(lua_State *L, int index)
        {
            if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                return (T)luaL_checknumber(L, index);
            } else if constexpr (std::is_integral_v<T>) {
                return (T)luaL_checkinteger(L, index);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return std::string(luaL_checkstring(L, index));
            } else {
                static_assert(sizeof(T) == 0, "Unsupported type in read_value");
            }
        }

        template <typename T, typename Member> Property makeProperty(const char *name, Member T::*member)
        {
            return Property{ name,
                             [member](lua_State *L, void *obj) {
                                 auto *compHandle = static_cast<ComponentHandle *>(obj);
                                 auto *component = static_cast<T *>(compHandle->component);
                                 push_value(L, component->*member);
                             }, // getter
                             [member](lua_State *L, void *obj) {
                                 auto *compHandle = static_cast<ComponentHandle *>(obj);
                                 auto *component = static_cast<T *>(compHandle->component);
                                 component->*member = read_value<Member>(L, 3);
                             } }; // setter
        }
    } // namespace luau
} // namespace atmo
