#include "types.hpp"

void atmo::core::types::register_core_types(flecs::world ecs)
{
    ecs.component<Vector2>().member<float>("x").member<float>("y");
    ecs.component<Vector2i>().member<int>("x").member<int>("y");
    ecs.component<Vector3>().member<float>("x").member<float>("y").member<float>("z");
    ecs.component<Vector3i>().member<int>("x").member<int>("y").member<int>("z");
    ecs.component<Vector4>().member<float>("x").member<float>("y").member<float>("z").member<float>("w");
    ecs.component<Vector4i>().member<int>("x").member<int>("y").member<int>("z").member<int>("w");
    ecs.component<ColorRGBA>().member<float>("r").member<float>("g").member<float>("b").member<float>("a");
}
