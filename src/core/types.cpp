#include "types.hpp"

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
        atmo::meta::register_flecs_meta<atmo::core::types::ColorRGBA>(ecs);
        atmo::meta::register_flecs_meta<atmo::core::types::ColorRGBAi>(ecs);
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
