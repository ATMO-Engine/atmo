#include "core/types.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"
#include "luau/bindings/lua_bindings.hpp"


#include "bindings_color.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    Property LuaBindings<Color>::m_properties[] = { makeValueProperty<Color>("r", &Color::r),
                                                    makeValueProperty<Color>("g", &Color::g),
                                                    makeValueProperty<Color>("b", &Color::b),
                                                    makeValueProperty<Color>("a", &Color::a),
                                                    { nullptr, nullptr, nullptr } };
} // namespace atmo::luau

ATMO_REGISTER_LUA_COMPONENT_BINDING(atmo::core::types::Color);
