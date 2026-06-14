#include "bindings_transform2.hpp"

namespace atmo::luau
{
    static Property makeFloatProperty(const char *propName, float Transform2d::*member)
    {
        return Property{ propName,

                         [member](lua_State *L, void *obj) {
                             Transform2d *t = *(Transform2d **)obj;
                             push_value(L, t->*member);
                         },

                         [member](lua_State *L, void *obj) {
                             Transform2d *t = *(Transform2d **)obj;

                             t->*member = read_value<float>(L, 3);
                         } };
    }

    static Property makeVector2Property(const char *propName, Vector2 Transform2d::*member)
    {
        return Property{ propName,

                         [member](lua_State *L, void *obj) {
                             Transform2d *t = *(Transform2d **)obj;
                             void *mem = lua_newuserdata(L, sizeof(std::shared_ptr<Vector2>));
                             new (mem) std::shared_ptr<Vector2>(&(t->*member), [](Vector2 *) {});
                             luaL_getmetatable(L, LuaBindings<Vector2>::name);
                             lua_setmetatable(L, -2);
                         },

                         [member](lua_State *L, void *obj) {
                             Transform2d *t = *(Transform2d **)obj;
                             auto &vec = *(std::shared_ptr<Vector2> *)luaL_checkudata(L, 3, LuaBindings<Vector2>::name);
                             t->*member = *vec;
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
