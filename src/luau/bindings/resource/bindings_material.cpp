#include "bindings_material.hpp"

#include <array>
#include <cstring>
#include <optional>
#include <variant>
#include <vector>

#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/value/bindings_color.hpp"
#include "luau/bindings/value/bindings_vector2.hpp"

namespace atmo::luau
{
    namespace
    {
        using MaterialPtr = std::shared_ptr<core::resource::Material>;

        bool IsA(lua_State *state, int index, const char *name)
        {
            if (!lua_isuserdata(state, index) || !lua_getmetatable(state, index))
                return false;
            luaL_getmetatable(state, name);
            const bool same = lua_rawequal(state, -1, -2);
            lua_pop(state, 2);
            return same;
        }

        std::optional<core::resource::MaterialValue> ReadValue(lua_State *state, int index, const core::resource::Material &material, const char *key)
        {
            switch (lua_type(state, index)) {
                case LUA_TNUMBER:
                    return lua_tonumber(state, index);
                case LUA_TBOOLEAN:
                    return lua_toboolean(state, index) ? 1.0 : 0.0;
                case LUA_TSTRING:
                    {
                        core::resource::MaterialTextureRef texture;
                        if (auto current = material.get(key))
                            if (const auto *ref = std::get_if<core::resource::MaterialTextureRef>(&*current))
                                texture = *ref;
                        texture.path = lua_tostring(state, index);
                        return texture;
                    }
                case LUA_TTABLE:
                    {
                        std::vector<float> numbers;
                        const int count = lua_objlen(state, index);
                        for (int i = 1; i <= count; ++i) {
                            lua_rawgeti(state, index, i);
                            if (!lua_isnumber(state, -1)) {
                                lua_pop(state, 1);
                                return std::nullopt;
                            }
                            numbers.push_back(static_cast<float>(lua_tonumber(state, -1)));
                            lua_pop(state, 1);
                        }
                        if (numbers.size() == 2)
                            return std::array{ numbers[0], numbers[1] };
                        if (numbers.size() == 3)
                            return std::array{ numbers[0], numbers[1], numbers[2] };
                        if (numbers.size() == 4)
                            return std::array{ numbers[0], numbers[1], numbers[2], numbers[3] };
                        return std::nullopt;
                    }
                case LUA_TUSERDATA:
                    if (IsA(state, index, LuaBindings<core::types::Vector2>::name)) {
                        const auto *vector = LuaBindings<core::types::Vector2>::CheckPtr(state, index);
                        return std::array{ vector->x, vector->y };
                    }
                    if (IsA(state, index, LuaBindings<core::types::Color>::name)) {
                        const auto *color = LuaBindings<core::types::Color>::CheckPtr(state, index);
                        return std::array{ color->r, color->g, color->b, color->a };
                    }
                    return std::nullopt;
                default:
                    return std::nullopt;
            }
        }

        template <std::size_t N> void PushNumbers(lua_State *state, const std::array<float, N> &numbers)
        {
            lua_createtable(state, static_cast<int>(N), 0);
            for (std::size_t i = 0; i < N; ++i) {
                lua_pushnumber(state, numbers[i]);
                lua_rawseti(state, -2, static_cast<int>(i + 1));
            }
        }
    } // namespace

    Property LuaBindings<MaterialPtr>::m_properties[] = { { nullptr, nullptr, nullptr } };

    int LuaBindings<MaterialPtr>::Set(lua_State *state)
    {
        const MaterialPtr &material = *CheckPtr(state, 1);
        const char *key = luaL_checkstring(state, 2);
        if (!material) {
            lua_pushboolean(state, false);
            return 1;
        }

        std::optional<core::resource::MaterialValue> value = ReadValue(state, 3, *material, key);
        if (!value)
            luaL_error(state, "Material.set: unsupported value for '%s' (use a number, Vector2, Color, a table of 2 to 4 numbers or a texture path)", key);
        lua_pushboolean(state, material->set(key, std::move(*value)));
        return 1;
    }

    int LuaBindings<MaterialPtr>::Get(lua_State *state)
    {
        const MaterialPtr &material = *CheckPtr(state, 1);
        const char *key = luaL_checkstring(state, 2);
        std::optional<core::resource::MaterialValue> value = material ? material->get(key) : std::nullopt;
        if (!value) {
            lua_pushnil(state);
            return 1;
        }

        const server::rendering::ShaderParam *param = material->shader() ? material->shader()->layout().find(key) : nullptr;
        std::visit(
            [&](const auto &v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>) {
                    lua_pushnumber(state, v);
                } else if constexpr (std::is_same_v<T, core::resource::MaterialTextureRef>) {
                    lua_pushstring(state, v.path.c_str());
                } else if constexpr (std::is_same_v<T, std::array<float, 2>>) {
                    LuaBindings<core::types::Vector2>::Push(state, core::types::Vector2(v[0], v[1]));
                } else if constexpr (std::is_same_v<T, std::array<float, 4>>) {
                    if (param && param->type == server::rendering::ShaderParamType::Color)
                        LuaBindings<core::types::Color>::Push(state, core::types::Color(v[0], v[1], v[2], v[3]));
                    else
                        PushNumbers(state, v);
                } else {
                    PushNumbers(state, v);
                }
            },
            *value);
        return 1;
    }

    int LuaBindings<MaterialPtr>::GetShader(lua_State *state)
    {
        const MaterialPtr &material = *CheckPtr(state, 1);
        lua_pushstring(state, material ? material->shaderPath().c_str() : "");
        return 1;
    }
} // namespace atmo::luau

ATMO_REGISTER_LUA_COMPONENT_BINDING(std::shared_ptr<atmo::core::resource::Material>);
