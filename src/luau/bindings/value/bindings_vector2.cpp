#include "bindings_vector2.hpp"
#include "core/types.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/lua_bindings.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    Property LuaBindings<Vector2>::m_properties[] = { makeValueProperty<Vector2>("x", &Vector2::x),
                                                      makeValueProperty<Vector2>("y", &Vector2::y),
                                                      { nullptr, nullptr, nullptr } };
} // namespace atmo::luau

ATMO_REGISTER_LUA_COMPONENT_BINDING(atmo::core::types::Vector2);
