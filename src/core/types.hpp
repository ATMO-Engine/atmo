#pragma once

#include <string>

#include "flecs.h"

namespace atmo
{
    namespace core
    {
        namespace types
        {
            typedef struct Vector2 {
                float x = 0.0f;
                float y = 0.0f;
            } vector2;

            typedef struct Vector2i {
                int x = 0;
                int y = 0;
            } vector2i;

            typedef struct Vector3 {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
            } vector3;

            typedef struct Vector3i {
                int x = 0;
                int y = 0;
                int z = 0;
            } vector3i;

            typedef struct Vector4 {
                float x = 0.0f;
                float y = 0.0f;
                float z = 0.0f;
                float w = 0.0f;
            } vector4;

            typedef struct Rgba {
                float r = 1.0f;
                float g = 1.0f;
                float b = 1.0f;
                float a = 1.0f;
            } rgba;

            typedef struct Vector4i {
                int x = 0;
                int y = 0;
                int z = 0;
                int w = 0;
            } vector4i;

            static void register_core_types(flecs::world ecs)
            {
                ecs.component<types::vector2>().member<float>("x").member<float>("y");
                ecs.component<types::vector2i>().member<int>("x").member<int>("y");
                ecs.component<types::vector3>().member<float>("x").member<float>("y").member<float>("z");
                ecs.component<types::vector3i>().member<int>("x").member<int>("y").member<int>("z");
                ecs.component<types::vector4>().member<float>("x").member<float>("y").member<float>("z").member<float>("w");
                ecs.component<types::vector4i>().member<int>("x").member<int>("y").member<int>("z").member<int>("w");
                ecs.component<types::rgba>().member<float>("r").member<float>("g").member<float>("b").member<float>("a");
            }
        } // namespace types
    } // namespace core
} // namespace atmo
