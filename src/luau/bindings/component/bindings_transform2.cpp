#include "bindings_transform2.hpp"
#include "luau/bindings/lua_bindings.hpp"
#include "luau/bindings/value/bindings_vector2.hpp"
#include "luau/bindings/EntityLuauRegistry.hpp"

namespace atmo::luau
{
    Property LuaBindings<Transform2d>::m_properties[] = { makeVector2Property("position", &Transform2d::position),
                                                          makeVector2Property("g_position", &Transform2d::g_position),
                                                          makeVector2Property("scale", &Transform2d::scale),
                                                          makeVector2Property("g_scale", &Transform2d::g_scale),
                                                          makeFloatProperty("rotation", &Transform2d::rotation),
                                                          makeFloatProperty("g_rotation", &Transform2d::g_rotation),
                                                          { nullptr, nullptr, nullptr } };
} // namespace atmo::luau

ATMO_REGISTER_LUA_COMPONENT_BINDING(atmo::core::components::Transform2d);
