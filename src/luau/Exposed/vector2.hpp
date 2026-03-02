#pragma once

#include <memory>
#include "lua.h"

namespace atmo {
    namespace luau {
        class Vector2 {
            public:
                Vector2();
                Vector2(float x, float y);
                ~Vector2() = default;

                static void RegisterVector2(lua_State *state);

                float length() const;

                void set(float x, float y);
                void setX(float x);
                void setY(float y);

                float getX() const;
                float getY() const;
            private:
                static int Vector2New(lua_State *state);
                static std::shared_ptr<Vector2> &CheckVector2(lua_State* L, int index);
                static int Vector2Length(lua_State *state);
                static int Vector2Index(lua_State *state);
                static int Vector2NewIndex(lua_State *state);
                static int Vector2GC(lua_State *state);

                float m_x = 0;
                float m_y = 0;
        };
    } // namespace luau
} // namespace atmo
