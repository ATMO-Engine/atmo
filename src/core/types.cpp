#include "types.hpp"

namespace atmo::core::types
{
    void register_core_types(flecs::world ecs)
    {
        ecs.component<Vector2>().member<float>("x").member<float>("y");
        ecs.component<Vector2i>().member<int>("x").member<int>("y");
        ecs.component<Vector3>().member<float>("x").member<float>("y").member<float>("z");
        ecs.component<Vector3i>().member<int>("x").member<int>("y").member<int>("z");
        ecs.component<Vector4>().member<float>("x").member<float>("y").member<float>("z").member<float>("w");
        ecs.component<Vector4i>().member<int>("x").member<int>("y").member<int>("z").member<int>("w");
        ecs.component<ColorRGBA>().member<float>("r").member<float>("g").member<float>("b").member<float>("a");
        ecs.component<ColorRGBAi>().member<std::uint8_t>("r").member<std::uint8_t>("g").member<std::uint8_t>("b").member<std::uint8_t>("a");
    }

    ColorRGBA::ColorRGBA(const ColorRGBAi &c)
    {
        r = c.r / 255.0f;
        g = c.g / 255.0f;
        b = c.b / 255.0f;
        a = c.a / 255.0f;
    }

    ColorRGBA::operator ColorRGBAi() const
    {
        return ColorRGBAi(*this);
    }

    ColorRGBAi::ColorRGBAi(const ColorRGBA &c)
    {
        r = static_cast<uint8_t>(common::math::Clamp(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
        g = static_cast<uint8_t>(common::math::Clamp(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
        b = static_cast<uint8_t>(common::math::Clamp(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
        a = static_cast<uint8_t>(common::math::Clamp(c.a, 0.0f, 1.0f) * 255.0f + 0.5f);
    }
} // namespace atmo::core::types
