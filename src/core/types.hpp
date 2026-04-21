#pragma once

#include <cstdint>
#include <string>

#include "SDL3/SDL_pixels.h"
#include "box2d/box2d.h"
#include "common/math.hpp"
#include "flecs.h"
#include "meta/meta.hpp"

namespace atmo::core::types
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

        Vector2 operator/(float f) const
        {
            return Vector2(x / f, y / f);
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

    struct ColorRGBAi;

    struct ColorRGBA {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        ColorRGBA() = default;
        ColorRGBA(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
        explicit ColorRGBA(const ColorRGBAi &c);
        explicit operator ColorRGBAi() const;

        SDL_FColor toSDLColor() const
        {
            return SDL_FColor{ r, g, b, a };
        }
    };

    struct ColorRGBAi {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;

        ColorRGBAi() = default;
        ColorRGBAi(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) : r(r), g(g), b(b), a(a) {}
        explicit ColorRGBAi(const ColorRGBA &c);
    };

    void register_core_types(flecs::world ecs);
} // namespace atmo::core::types

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector2> {
    static constexpr const char *name = "Vector2";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::types::Vector2::x>("x"), atmo::meta::field<&atmo::core::types::Vector2::y>("y"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector2i> {
    static constexpr const char *name = "Vector2i";
    static constexpr auto fields =
        std::make_tuple(atmo::meta::field<&atmo::core::types::Vector2i::x>("x"), atmo::meta::field<&atmo::core::types::Vector2i::y>("y"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector3> {
    static constexpr const char *name = "Vector3";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector3::x>("x"), atmo::meta::field<&atmo::core::types::Vector3::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector3::z>("z"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector3i> {
    static constexpr const char *name = "Vector3i";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector3i::x>("x"), atmo::meta::field<&atmo::core::types::Vector3i::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector3i::z>("z"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector4> {
    static constexpr const char *name = "Vector4";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector4::x>("x"), atmo::meta::field<&atmo::core::types::Vector4::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector4::z>("z"), atmo::meta::field<&atmo::core::types::Vector4::w>("w"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Vector4i> {
    static constexpr const char *name = "Vector4i";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Vector4i::x>("x"), atmo::meta::field<&atmo::core::types::Vector4i::y>("y"),
        atmo::meta::field<&atmo::core::types::Vector4i::z>("z"), atmo::meta::field<&atmo::core::types::Vector4i::w>("w"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::ColorRGBA> {
    static constexpr const char *name = "ColorRGBA";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::ColorRGBA::r>("r"), atmo::meta::field<&atmo::core::types::ColorRGBA::g>("g"),
        atmo::meta::field<&atmo::core::types::ColorRGBA::b>("b"), atmo::meta::field<&atmo::core::types::ColorRGBA::a>("a"));
};

template <> struct atmo::meta::ComponentMeta<atmo::core::types::ColorRGBAi> {
    static constexpr const char *name = "ColorRGBAi";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::ColorRGBAi::r>("r"), atmo::meta::field<&atmo::core::types::ColorRGBAi::g>("g"),
        atmo::meta::field<&atmo::core::types::ColorRGBAi::b>("b"), atmo::meta::field<&atmo::core::types::ColorRGBAi::a>("a"));
};
