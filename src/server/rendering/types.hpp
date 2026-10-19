#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.hpp"

namespace atmo::server::rendering
{
    struct Rect2 {
        core::types::Vector2 position{ 0.0f, 0.0f };
        core::types::Vector2 size{ 0.0f, 0.0f };

        [[nodiscard]] static constexpr Rect2 Centered(core::types::Vector2 size) noexcept
        {
            return { { -size.x * 0.5f, -size.y * 0.5f }, size };
        }
    };

    struct Affine2d {
        float a = 1.0f;
        float b = 0.0f;
        float c = 0.0f;
        float d = 1.0f;
        float tx = 0.0f;
        float ty = 0.0f;

        [[nodiscard]] static constexpr Affine2d Identity() noexcept
        {
            return {};
        }

        [[nodiscard]] static constexpr Affine2d Translation(core::types::Vector2 offset) noexcept
        {
            return { .tx = offset.x, .ty = offset.y };
        }

        [[nodiscard]] static constexpr Affine2d Scaling(core::types::Vector2 scale) noexcept
        {
            return { .a = scale.x, .d = scale.y };
        }

        [[nodiscard]] static Affine2d Rotation(float radians) noexcept
        {
            const float cos = std::cos(radians);
            const float sin = std::sin(radians);
            return { .a = cos, .b = sin, .c = -sin, .d = cos };
        }

        [[nodiscard]] static Affine2d FromTrs(core::types::Vector2 position, float rotation_degrees, core::types::Vector2 scale) noexcept
        {
            return Translation(position) * Rotation(rotation_degrees * std::numbers::pi_v<float> / 180.0f) * Scaling(scale);
        }

        [[nodiscard]] constexpr core::types::Vector2 apply(core::types::Vector2 point) const noexcept
        {
            return { (a * point.x) + (c * point.y) + tx, (b * point.x) + (d * point.y) + ty };
        }

        [[nodiscard]] constexpr Affine2d operator*(const Affine2d &rhs) const noexcept
        {
            return {
                .a = (a * rhs.a) + (c * rhs.b),
                .b = (b * rhs.a) + (d * rhs.b),
                .c = (a * rhs.c) + (c * rhs.d),
                .d = (b * rhs.c) + (d * rhs.d),
                .tx = (a * rhs.tx) + (c * rhs.ty) + tx,
                .ty = (b * rhs.tx) + (d * rhs.ty) + ty,
            };
        }

        constexpr bool operator==(const Affine2d &) const noexcept = default;
    };

    using ColorRgba = std::array<float, 4>;

    inline constexpr ColorRgba white{ 1.0f, 1.0f, 1.0f, 1.0f };

    enum class TextureFilter : std::uint8_t {
        Linear,
        Nearest,
    };

    struct TextureRectOptions {
        std::optional<Rect2> source;
        ColorRgba modulate = white;
        TextureFilter filter = TextureFilter::Linear;
        bool repeat = false;
    };

    struct Texture2dDesc {
        core::types::Vector2i size{};
        std::string debug_name;
    };

    struct TextureData {
        core::types::Vector2i size{};
        std::vector<std::byte> pixels;
    };

    struct FrameStats {
        std::uint64_t frame = 0;
        std::uint32_t draw_calls = 0;
        std::uint32_t instances = 0;
        std::uint32_t vertices = 0;
        std::uint32_t canvas_items = 0;
        std::uint32_t culled_items = 0;
        double render_cpu_ms = 0.0;
    };

    enum class PlatformKind : std::uint8_t {
        Null,
        Vulkan,
        Metal,
    };

    enum class ShaderTarget : std::uint8_t {
        None,
        SpirV,
        Msl,
        MetalLib,
        Dxil,
    };

    [[nodiscard]] std::string_view PlatformKindName(PlatformKind kind) noexcept;
    [[nodiscard]] std::optional<PlatformKind> ParsePlatformKind(std::string_view name) noexcept;

    struct RenderingSettings {
        std::optional<PlatformKind> platform;
        bool inline_commands = false;
        bool render_thread = true;
        bool legacy_sdl_renderer = true;
        bool gpu_validation = false;
    };

    struct WindowDesc {
        std::string title;
        core::types::Vector2i size;
        bool resizable = true;
    };

    enum class WindowEventType : std::uint8_t {
        QuitRequested,
        Resized,
        DisplayScaleChanged,
    };

    struct WindowEvent {
        WindowEventType type = WindowEventType::QuitRequested;
        core::types::Vector2i size{};
    };
} // namespace atmo::server::rendering
