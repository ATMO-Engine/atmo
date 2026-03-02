#pragma once

#include "vector2.hpp"
#include <cmath>
#include <memory>
#include <cstring>
#include "lua.h"
#include "lualib.h"

namespace atmo
{
    namespace luau
    {
        void Vector2::RegisterVector2(lua_State *state)
        {
            luaL_newmetatable(state, "Vector2");

            lua_pushcfunction(state, Vector2::Vector2GC, "Vector2.__gc");
            lua_setfield(state, -2, "__gc");

            lua_pushcfunction(state, Vector2::Vector2Index, "Vector2.__index");
            lua_setfield(state, -2, "__index");

            lua_pushcfunction(state, Vector2::Vector2NewIndex, "Vector2.__newindex");
            lua_setfield(state, -2, "__newindex");

            lua_newtable(state);

            lua_pushcfunction(state, Vector2::Vector2Length, "Vector2.length");
            lua_setfield(state, -2, "length");

            lua_setfield(state, -2, "__methods");

            lua_pop(state, 1);

            lua_pushcfunction(state, Vector2::Vector2New, "Vector2.__new");

            lua_setglobal(state, "Vector2");
        }

        int Vector2::Vector2New(lua_State *state)
        {
            float x = (float)luaL_checknumber(state, 1);
            float y = (float)luaL_checknumber(state, 2);

            void *mem = lua_newuserdata(state, sizeof(std::shared_ptr<Vector2>));
            new (mem) std::shared_ptr<Vector2>(std::make_shared<Vector2>(x, y));

            luaL_getmetatable(state, "Vector2");
            lua_setmetatable(state, -2);

            return 1;
        }

        std::shared_ptr<Vector2> &Vector2::CheckVector2(lua_State *L, int index)
        {
            return *(std::shared_ptr<Vector2> *)luaL_checkudata(L, index, "Vector2");
        }

        int Vector2::Vector2Length(lua_State *state)
        {
            auto &v = Vector2::CheckVector2(state, 1);
            lua_pushnumber(state, v->length());
            return 1;
        }

        int Vector2::Vector2Index(lua_State *state)
        {
            auto &v = CheckVector2(state, 1);
            const char *key = luaL_checkstring(state, 2);

            // Properties
            if (strcmp(key, "x") == 0) {
                lua_pushnumber(state, v->getX());
                return 1;
            }
            if (strcmp(key, "y") == 0) {
                lua_pushnumber(state, v->getY());
                return 1;
            }

            // Methods lookup
            luaL_getmetatable(state, "Vector2");
            lua_getfield(state, -1, "__methods");
            lua_getfield(state, -1, key);

            return 1;
        }

        int Vector2::Vector2NewIndex(lua_State *state)
        {
            auto &v = CheckVector2(state, 1);
            const char *key = luaL_checkstring(state, 2);
            float value = (float)luaL_checknumber(state, 3);

            if (strcmp(key, "x") == 0) {
                v->setX(value);
                return 0;
            }
            if (strcmp(key, "y") == 0) {
                v->setY(value);
                return 0;
            }

            return 0;
        }

        int Vector2::Vector2GC(lua_State *state)
        {
            auto *ptr = (std::shared_ptr<Vector2> *)luaL_checkudata(state, 1, "Vector2");

            ptr->~shared_ptr<Vector2>();
            return 0;
        }

        Vector2::Vector2()
        {
            m_x = 0;
            m_y = 0;
        }

        Vector2::Vector2(float x, float y)
        {
            m_x = x;
            m_y = y;
        }

        float Vector2::length() const
        {
            return std::sqrt(m_x * m_x + m_y * m_y);
        }

        void Vector2::set(float x, float y)
        {
            m_x = x;
            m_y = y;
        }

        void Vector2::setX(float x)
        {
            m_x = x;
        }

        void Vector2::setY(float y)
        {
            m_y = y;
        }

        float Vector2::getX() const
        {
            return m_x;
        }

        float Vector2::getY() const
        {
            return m_y;
        }
    } // namespace luau
} // namespace atmo
