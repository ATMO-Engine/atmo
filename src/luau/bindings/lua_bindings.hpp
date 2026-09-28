#pragma once

#include <functional>
#include <initializer_list>
#include "lua.h"
#include "lualib.h"

#include "spdlog/spdlog.h"

namespace atmo
{
    namespace luau
    {
        struct Property {
            const char *name;

            std::function<void(lua_State *, void *)> getter;
            std::function<void(lua_State *, void *)> setter;
        };

        template <typename Derived, typename T, typename ParentBinding = void> class LuaBindingsBase
        {
        public:

            using Parent = ParentBinding;

            static constexpr const char *ParentName()
            {
                if constexpr (std::is_void_v<ParentBinding>) {
                    return nullptr;
                } else {
                    return ParentBinding::name;
                }
            }

            /**
             * @brief
             * Get a T* from the luau
             *
             * @param L The vm / execution context in which you perform the action
             * @param index the index in the stack for the T object
             * @return T* the object requested, always check the value returned
             */
            static T *CheckPtr(lua_State *L, int index)
            {
                return Derived::CheckPtrImpl(L, index);
            }
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
                luaL_checkudata(L, 1, Derived::name);
                const char *key = luaL_checkstring(L, 2);

                luaL_getmetatable(L, Derived::name);

                lua_getfield(L, -1, "__properties");
                if (lua_istable(L, -1))
                {
                    lua_getfield(L, -1, key);
                    if (!lua_isnil(L, -1))
                    {
                        auto *prop = static_cast<Property *>(lua_touserdata(L, -1));
                        lua_pop(L, 3);
                        if (!prop->getter) {
                            luaL_error(L, "Property '%s.%s' has no getter", Derived::name, key);
                        }
                        prop->getter(L, Derived::CheckPtr(L, 1));
                        return 1;
                    }
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);

                lua_getfield(L, -1, "__methods");
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 2);
                    lua_pushnil(L);
                    return 1;
                }
                lua_getfield(L, -1, key);
                lua_remove(L, -2);
                lua_remove(L, -2);
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
                luaL_checkudata(L, 1, Derived::name);
                const char *key = luaL_checkstring(L, 2);

                luaL_getmetatable(L, Derived::name);
                lua_getfield(L, -1, "__properties");
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, key);
                    if (!lua_isnil(L, -1))
                    {
                        Property *prop = static_cast<Property *>(lua_touserdata(L, -1));
                        lua_pop(L, 3);
                        if (!prop->setter) {
                            luaL_errorL(L, "Property '%s.%s' is read-only", Derived::name, key);
                        }
                        prop->setter(L, Derived::CheckPtr(L, 1));
                        return 0;
                    }
                }
                luaL_errorL(L, "'%s' is not a property of %s", key, Derived::name);
            }

            /**
             * @brief
             * Prevent scripts from reading or tampering with the metatable: getmetatable() returns
             * a string instead of the table, and the table and its sub-tables are made read-only.
             * Must be called last, once the metatable is fully built.
             * Precondition: [-1] = metatable being built.
             */
            static void Seal(lua_State *L)
            {
                for (const char *field : { "__methods", "__properties", "__isa" }) {
                    lua_getfield(L, -1, field);
                    if (lua_istable(L, -1))
                        lua_setreadonly(L, -1, true);
                    lua_pop(L, 1);
                }
                lua_pushstring(L, "The metatable is locked");
                lua_setfield(L, -2, "__metatable");
                lua_setreadonly(L, -1, true);
            }

            static void RegisterProperties(lua_State *L)
            {
                lua_newtable(L);
                for (int i = 0; Derived::m_properties[i].name; i++)
                {
                    if (!Derived::m_properties[i].getter) {
                        luaL_error(L, "Property '%s.%s' has no getter", Derived::name, Derived::m_properties[i].name);
                    }
                    if (!Derived::m_properties[i].setter) {
                        spdlog::debug("Property '{}.{}' is read only", Derived::name, Derived::m_properties[i].name);
                    }

                    lua_pushlightuserdata(L, &Derived::m_properties[i]);
                    lua_setfield(L, -2, Derived::m_properties[i].name);
                }
                if constexpr (!std::is_void_v<ParentBinding>)
                    ChainTable(L, ParentBinding::name, "__properties");
                lua_setfield(L, -2, "__properties"); // metatable.__properties = table
            }

            /**
             * @brief
             * Make entity fallback to parent table if the entity table doesn't have what the user want
             * @param L The vm / execution context in which you perform the action
             * @param parentName The name of the parent binding
             * @param field The filed of the parent you want to fallback
             */
            static void ChainTable(lua_State *L, const char *parentName, const char *field)
            {
                luaL_getmetatable(L, parentName);
                if (lua_isnil(L, -1)) {
                    luaL_error(L, "parent %s not registered before child", parentName);
                }
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 1);
                    luaL_error( L, "Parent '%s' metatable is invalid", parentName );
                }

                lua_getfield(L, -1, field);
                lua_remove(L, -2);
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 1);
                    luaL_error( L, "Parent '%s.%s' is not a table", parentName, field );
                }

                lua_createtable(L, 0, 1);
                lua_insert(L, -2);
                lua_setfield(L, -2, "__index");
                lua_setmetatable(L, -2);
            }

            /**
             * @brief
             * Registers this type's ancestry so CheckIsA can validate userdata
             * across inherited methods (e.g. Entity::getTransform called on a
             * Dynamic2d instance).
             * Precondition: [-1] = metatable being built.
             */
            static void RegisterIsA(lua_State *L)
            {
                lua_newtable(L);                                                    // [-1] isa table
                lua_pushboolean(L, true);
                lua_setfield(L, -2, Derived::name);                          // isa[Derived::name] = true
                if constexpr (!std::is_void_v<ParentBinding>) {
                    ChainTable(L, ParentBinding::name, "__isa");
                }
                lua_setfield(L, -2, "__isa");                                // metatable.__isa = isa table
            }

            /**
             * @brief
             * Checks that the userdata at `index` is `expectedName` or a descendant
             * of it (per __isa chain), without requiring an exact metatable match.
             */
            static bool CheckIsA(lua_State *L, int index, const char *expectedName)
            {
                if (!lua_isuserdata(L, index)) {
                    return false;
                }
                if (!lua_getmetatable(L, index)) {
                    return false;
                }

                lua_getfield(L, -1, "__isa");
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 2);
                    return false;
                }
                lua_getfield(L, -1, expectedName);
                bool ok = lua_toboolean(L, -1);
                lua_pop(L, 3);
                return ok;
            }
        };

        template <typename T> class LuaBindings : public LuaBindingsBase<LuaBindings<T>, T>
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
                                 auto *component = static_cast<T *>(obj);
                                 push_value(L, component->*member);
                             },
                             [member](lua_State *L, void *obj) {
                                 auto *component = static_cast<T *>(obj);
                                 component->*member = read_value<Member>(L, 3);
                             } };
        }

        /**
         * @brief
         * Property of a value type (Vector2, Color...): readable, but writing it raises an error.
         * The value is a copy, so `transform.position.x = 1` would silently modify nothing.
         */
        template <typename T, typename Member> Property makeValueProperty(const char *name, Member T::*member)
        {
            return Property{ name,
                             [member](lua_State *L, void *obj) {
                                 push_value(L, static_cast<T *>(obj)->*member);
                             },
                             [name](lua_State *L, void *) {
                                 luaL_errorL(L,
                                             "'%s' can't be modified, this value is a copy: create a new one instead "
                                             "(e.g. transform.position = Vector2(x, y))",
                                             name);
                             } };
        }

        template <typename T> Property makeFloatProperty(const char *name, float T::*member)
        {
            return Property{ name,
                             [member](lua_State *L, void *obj) {
                                 auto *component = static_cast<T *>(obj);
                                 push_value(L, component->*member);
                             },
                             [member](lua_State *L, void *obj) {
                                 auto *component = static_cast<T *>(obj);
                                 component->*member = read_value<float>(L, 3);
                             } };
        }

        template <typename T, typename VecT> Property makeVector2Property(const char *name, VecT T::*member)
        {
            return Property{ name,
                             [member](lua_State *L, void *obj) {
                                 auto *component = static_cast<T *>(obj);
                                 LuaBindings<VecT>::Push(L, component->*member); // copy
                             },
                             [member](lua_State *L, void *obj) {
                                 auto *component = static_cast<T *>(obj);
                                 component->*member = *LuaBindings<VecT>::CheckPtr(L, 3);
                             } };
        }
    } // namespace luau
} // namespace atmo
