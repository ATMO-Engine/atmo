#include "core/types.hpp"
#include "lua_bindings.hpp"

#include "bindings_vector2.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    Property LuaBindings<Vector2>::m_properties[] = { makeProperty<Vector2>("x", &Vector2::x),
                                                      makeProperty<Vector2>("y", &Vector2::y),
                                                      { nullptr, nullptr, nullptr } };
} // namespace atmo::luau
