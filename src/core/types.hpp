#pragma once

#include <string>

#include "box2d/box2d.h"
#include "common/math.hpp"
#include "flecs.h"

namespace atmo
{
    namespace core
    {
        namespace types
        {
            struct Vector2 {
                float x = 0.0f;
                float y = 0.0f;

                Vector2() = default;
                Vector2(float x, float y) : x(x), y(y) {}

                Vector2(const b2Vec2 &v) : x(common::math::MeterToPixel(v.x)), y(common::math::MeterToPixel(v.y)) {}
                operator b2Vec2() const
                {
                    return b2Vec2(common::math::PixelToMeter(x), common::math::PixelToMeter(y));
                }
            };

            struct Vector2i {
                int x = 0;
                int y = 0;
            };

            struct Vector3 {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
            };

            struct Vector3i {
                int x = 0;
                int y = 0;
                int z = 0;
            };

            struct Vector4 {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                float w = 0.0f;
            };

            struct Vector4i {
                int x = 0;
                int y = 0;
                int z = 0;
                int w = 0;
            };

            struct ColorRGBA {
                float r = 1.0f;
                float g = 1.0f;
                float b = 1.0f;
                float a = 1.0f;
            };

            void register_core_types(flecs::world ecs);
        } // namespace types
    } // namespace core
} // namespace atmo
