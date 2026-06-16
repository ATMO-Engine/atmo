#include "core/types.hpp"
#include "lua_bindings.hpp"

#include "bindings_color.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    Property LuaBindings<Color>::m_properties[] = { makeProperty<Color>("x", &Color::r),
                                                    makeProperty<Color>("y", &Color::g),
                                                    makeProperty<Color>("y", &Color::b),
                                                    makeProperty<Color>("y", &Color::a),
                                                    { nullptr, nullptr, nullptr } };
} // namespace atmo::luau
