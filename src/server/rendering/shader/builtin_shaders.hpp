#pragma once

#include <cstdint>
#include <optional>

#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    enum class BuiltinShader : std::uint8_t {
        TestPattern,
        Canvas,
    };

    [[nodiscard]] std::optional<ShaderBlob> BuiltinShaderBlob(BuiltinShader shader, ShaderTarget target) noexcept;
} // namespace atmo::server::rendering
