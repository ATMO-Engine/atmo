#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "server/rendering/rid.hpp"
#include "server/rendering/types.hpp"

namespace atmo::server::rendering
{
    enum class ShaderKind : std::uint8_t {
        Canvas,
        Viewport,
    };

    enum class ShaderParamType : std::uint8_t {
        Int,
        Uint,
        Float,
        Float2,
        Float3,
        Float4,
        Color,
        Texture,
    };

    [[nodiscard]] std::uint32_t ShaderParamSize(ShaderParamType type) noexcept;
    [[nodiscard]] std::string_view ShaderParamTypeName(ShaderParamType type) noexcept;
    [[nodiscard]] std::optional<ShaderParamType> ParseShaderParamType(std::string_view name) noexcept;
    [[nodiscard]] std::string_view ShaderKindName(ShaderKind kind) noexcept;
    [[nodiscard]] std::optional<ShaderKind> ParseShaderKind(std::string_view name) noexcept;

    struct ShaderParam {
        std::string name;
        ShaderParamType type = ShaderParamType::Float;
        std::uint32_t offset = 0;
        std::optional<std::array<float, 2>> range;
    };

    struct ShaderParamLayout {
        std::uint32_t size = 0;
        std::vector<ShaderParam> params;

        [[nodiscard]] const ShaderParam *find(std::string_view name) const noexcept;
    };

    struct ShaderCode {
        ShaderTarget target = ShaderTarget::None;
        std::vector<std::byte> code;
    };

    struct ShaderDesc {
        std::string name;
        ShaderKind kind = ShaderKind::Canvas;
        std::string entry_point;
        std::vector<ShaderCode> code;
        ShaderParamLayout layout;

        [[nodiscard]] const ShaderCode *codeFor(ShaderTarget target) const noexcept;
    };

    struct MaterialTexture {
        TextureRid texture;
        TextureFilter filter = TextureFilter::Linear;
        bool repeat = false;

        constexpr bool operator==(const MaterialTexture &) const noexcept = default;
    };

    using ShaderValue =
        std::variant<bool, std::int32_t, std::uint32_t, float, std::array<float, 2>, std::array<float, 3>, std::array<float, 4>, MaterialTexture>;

    struct MaterialTextureBinding {
        std::uint32_t offset = 0;
        MaterialTexture texture;
    };

    struct MaterialData {
        std::vector<std::byte> bytes;
        std::vector<MaterialTextureBinding> textures;
    };

    [[nodiscard]] bool WriteShaderValue(const ShaderParam &param, const ShaderValue &value, MaterialData &data);
} // namespace atmo::server::rendering
