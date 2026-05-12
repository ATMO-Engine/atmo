#include "types.hpp"

#include "common/math.hpp"
#include "meta/flecs_bridge.hpp"

namespace atmo::core::types
{
    void register_core_types(flecs::world ecs)
    {
        atmo::meta::register_flecs_meta<atmo::core::types::Vector2>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::Vector2i>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::Vector3>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::Vector3i>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::Vector4>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::Vector4i>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::Color>(ecs);
    }

    Color::Color(float r, float g, float b, float a)
    {
        this->r = common::math::Clamp(r, 0.0f, 1.0f);
        this->g = common::math::Clamp(g, 0.0f, 1.0f);
        this->b = common::math::Clamp(b, 0.0f, 1.0f);
        this->a = common::math::Clamp(a, 0.0f, 1.0f);
    }

    Color::Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
    {
        this->r = r / 255.0f;
        this->g = g / 255.0f;
        this->b = b / 255.0f;
        this->a = a / 255.0f;
    }

    Color Color::operator+(const Color &additive) const
    {
        return Color{ common::math::Clamp(r + additive.r, 0.0f, 1.0f),
                      common::math::Clamp(g + additive.g, 0.0f, 1.0f),
                      common::math::Clamp(b + additive.b, 0.0f, 1.0f),
                      common::math::Clamp(a + additive.a, 0.0f, 1.0f) };
    }

    Color Color::operator-(const Color &subtractive) const
    {
        return Color{ common::math::Clamp(r - subtractive.r, 0.0f, 1.0f),
                      common::math::Clamp(g - subtractive.g, 0.0f, 1.0f),
                      common::math::Clamp(b - subtractive.b, 0.0f, 1.0f),
                      common::math::Clamp(a - subtractive.a, 0.0f, 1.0f) };
    }

    Color Color::operator*(const Color &modulate) const
    {
        return Color{ r * modulate.r, g * modulate.g, b * modulate.b, a * modulate.a };
    }

    Color Color::operator/(const Color &divisive) const
    {
        return Color{ r / divisive.r, g / divisive.g, b / divisive.b, a / divisive.a };
    }

    Color Color::FromHex(std::uint32_t hex)
    {
        return Color{ static_cast<std::uint8_t>((hex >> 24) & 0xFF),
                      static_cast<std::uint8_t>((hex >> 16) & 0xFF),
                      static_cast<std::uint8_t>((hex >> 8) & 0xFF),
                      static_cast<std::uint8_t>(hex & 0xFF) };
    }

    const Color Color::WHITE{ 1.0f, 1.0f, 1.0f, 1.0f };
    const Color Color::BLACK{ 0.0f, 0.0f, 0.0f, 1.0f };
    const Color Color::RED{ 1.0f, 0.0f, 0.0f, 1.0f };
    const Color Color::GREEN{ 0.0f, 1.0f, 0.0f, 1.0f };
    const Color Color::BLUE{ 0.0f, 0.0f, 1.0f, 1.0f };
} // namespace atmo::core::types
