#pragma once

#include <functional>
#include <iostream>
#include "lua.h"
#include "lualib.h"

namespace atmo
{
    namespace luau
    {
        struct Property;

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
        protected:
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

                             // getter
                             [member](lua_State *L, void *obj) {
                                 auto &sp = *(std::shared_ptr<T> *)obj;
                                 push_value(L, sp.get()->*member);
                             },

                             // setter
                             [member](lua_State *L, void *obj) {
                                 auto &sp = *(std::shared_ptr<T> *)obj;
                                 sp.get()->*member = read_value<Member>(L, 3);
                             } };
        }
    } // namespace luau
} // namespace atmo
