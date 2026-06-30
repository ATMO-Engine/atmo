#include "bindings_transform2.hpp"
#include "bindings_vector2.hpp"

namespace atmo::luau
{
    static Property makeFloatProperty(const char *propName, float Transform2d::*member)
    {
        return Property{ propName,

                         [member](lua_State *L, void *obj) {
                             auto *compHandle = static_cast<ComponentHandle *>(obj);
                             auto *component = static_cast<Transform2d *>(compHandle->component);
                             push_value(L, component->*member);
                         },

                         [member](lua_State *L, void *obj) {
                             auto *compHandle = static_cast<ComponentHandle *>(obj);
                             auto *component = static_cast<Transform2d *>(compHandle->component);

                             component->*member = read_value<float>(L, 3);
                         } };
    }

    static Property makeVector2Property(const char *propName, Vector2 Transform2d::*member)
    {
        return Property{ propName,
                         [member](lua_State *L, void *obj) {
                             auto *compHandle = static_cast<ComponentHandle *>(obj);
                             auto *component = static_cast<Transform2d *>(compHandle->component);
                             LuaBindings<Vector2>::Push(L, &(component->*member));
                         },
                         [member](lua_State *L, void *obj) {
                             auto *compHandle = static_cast<ComponentHandle *>(obj);
                             auto *component = static_cast<Transform2d *>(compHandle->component);
                             auto *vec = LuaBindings<Vector2>::Check_ptr(L, 3);
                             if (!vec) {
                                 return;
                             }
                             component->*member = *vec;
                         } };
    }

    Property LuaBindings<Transform2d>::m_properties[] = { makeVector2Property("position", &Transform2d::position),
                                                          makeVector2Property("g_position", &Transform2d::g_position),
                                                          makeVector2Property("scale", &Transform2d::scale),
                                                          makeVector2Property("g_scale", &Transform2d::g_scale),
                                                          makeFloatProperty("rotation", &Transform2d::rotation),
                                                          makeFloatProperty("g_rotation", &Transform2d::g_rotation),
                                                          { nullptr, nullptr, nullptr } };
} // namespace atmo::luau
