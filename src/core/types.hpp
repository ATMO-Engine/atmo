#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include "SDL3/SDL_pixels.h"
#include "box2d/box2d.h"
#include "clay.h"
#include "common/math.hpp"
#include "flecs.h"
#include "meta/meta.hpp"
#include "spdlog/fmt/bundled/base.h"
#include "spdlog/fmt/bundled/format.h"

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

        float length() const
        {
            return std::sqrt(x * x + y * y);
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

    /**
     * @brief Color, can be converted to any format
     *
     */
    struct Color {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        Color(const Color &) = default;
        Color &operator=(const Color &) = default;

        Color(float r, float g, float b, float a);
        Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
        Color(std::string_view hex);

        Color operator+(const Color &modulator) const;
        Color operator-(const Color &modulator) const;
        Color operator*(const Color &modulator) const;
        Color operator/(const Color &modulator) const;

        static Color FromHex(std::uint32_t hex);

        template <typename T, typename Individual = std::uint8_t> T toInt(std::uint32_t range = 255) const
        {
            return T{ static_cast<Individual>(common::math::Round(r * range)),
                      static_cast<Individual>(common::math::Round(g * range)),
                      static_cast<Individual>(common::math::Round(b * range)),
                      static_cast<Individual>(common::math::Round(a * range)) };
        }

        template <typename T> T toFloat(float range = 1.0f) const
        {
            return T{ r * range, g * range, b * range, a * range };
        }

        static const Color WHITE;
        static const Color BLACK;
        static const Color RED;
        static const Color GREEN;
        static const Color BLUE;

    private:
        static float HexElementToFloat(std::string_view elem);
    };

    void register_core_types(flecs::world ecs);
} // namespace atmo::core::types

template <> struct fmt::formatter<atmo::core::types::Color> {
    enum class Mode {
        hex,
        rgb
    };

    Mode mode = Mode::hex;

    constexpr auto parse(fmt::format_parse_context &ctx)
    {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && *it != '}') {
            if (*it == 'r') {
                mode = Mode::rgb;
                ++it;
            } else if (*it == 'h') {
                mode = Mode::hex;
                ++it;
            } else {
                throw fmt::format_error("invalid color format");
            }
        }

        return it;
    }

    template <typename FormatContext> auto format(const atmo::core::types::Color &c, FormatContext &ctx) const
    {
        auto out = ctx.out();

        auto toByte = [](float v) { return static_cast<unsigned>(v * 255.0f + 0.5f); };

        switch (mode) {
            case Mode::hex:
                return fmt::format_to(out, "#{:02X}{:02X}{:02X}{:02X}", toByte(c.r), toByte(c.g), toByte(c.b), toByte(c.a));

            case Mode::rgb:
                return fmt::format_to(out, "rgba({}, {}, {}, {:.2f})", toByte(c.r), toByte(c.g), toByte(c.b), c.a);
        }

        return out;
    }
};

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

template <> struct atmo::meta::ComponentMeta<atmo::core::types::Color> {
    static constexpr const char *name = "Color";
    static constexpr auto fields = std::make_tuple(
        atmo::meta::field<&atmo::core::types::Color::r>("r"), atmo::meta::field<&atmo::core::types::Color::g>("g"),
        atmo::meta::field<&atmo::core::types::Color::b>("b"), atmo::meta::field<&atmo::core::types::Color::a>("a"));
};
