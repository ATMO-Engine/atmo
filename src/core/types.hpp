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

            enum Shape2dType {
                None,
                Rectangle,
                Circle,
                Capsule,
                Polygon
            };

            static void register_core_types(flecs::world ecs)
            {
                ecs.component<Vector2>().member<float>("x").member<float>("y");
                ecs.component<Vector2i>().member<int>("x").member<int>("y");
                ecs.component<Vector3>().member<float>("x").member<float>("y").member<float>("z");
                ecs.component<Vector3i>().member<int>("x").member<int>("y").member<int>("z");
                ecs.component<Vector4>().member<float>("x").member<float>("y").member<float>("z").member<float>("w");
                ecs.component<Vector4i>().member<int>("x").member<int>("y").member<int>("z").member<int>("w");
                ecs.component<ColorRGBA>().member<float>("r").member<float>("g").member<float>("b").member<float>("a");
                ecs.component<Shape2dType>()
                    .constant("None", Shape2dType::None)
                    .constant("Rectangle", Shape2dType::Rectangle)
                    .constant("Circle", Shape2dType::Circle)
                    .constant("Capsule", Shape2dType::Capsule)
                    .constant("Polygon", Shape2dType::Polygon);
            }
        } // namespace types
    } // namespace core
} // namespace atmo
