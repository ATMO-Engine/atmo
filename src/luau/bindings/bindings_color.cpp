#include "core/types.hpp"
#include "lua_bindings.hpp"

#include "bindings_color.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    Property LuaBindings<Color>::m_properties[] = { makeProperty<Color>("r", &Color::r),
                                                    makeProperty<Color>("g", &Color::g),
                                                    makeProperty<Color>("b", &Color::b),
                                                    makeProperty<Color>("a", &Color::a),
                                                    { nullptr, nullptr, nullptr } };
} // namespace atmo::luau
