#include "core/types.hpp"
#include "lua_bindings.hpp"

#include "bindings_color.hpp"

namespace atmo::luau
{
    using namespace atmo::core::types;

    Property LuaBindings<ColorRGBA>::m_properties[] = { makeProperty<ColorRGBA>("x", &ColorRGBA::r),
                                                        makeProperty<ColorRGBA>("y", &ColorRGBA::g),
                                                        makeProperty<ColorRGBA>("y", &ColorRGBA::b),
                                                        makeProperty<ColorRGBA>("y", &ColorRGBA::a),
                                                        { nullptr, nullptr, nullptr } };
} // namespace atmo::luau
