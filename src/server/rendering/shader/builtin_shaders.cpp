#include "builtin_shaders.hpp"

#include <cstddef>
#include <span>

namespace atmo::server::rendering
{
    namespace
    {
#if defined(ATMO_RENDERING_VULKAN)
        alignas(4) constexpr unsigned char test_pattern_spirv[] = {
#include "shaders/test_pattern.spirv.h"
        };

        alignas(4) constexpr unsigned char canvas_spirv[] = {
#include "shaders/canvas.spirv.h"
        };

        alignas(4) constexpr unsigned char post_spirv[] = {
#include "shaders/post.spirv.h"
        };
#endif

#if defined(ATMO_RENDERING_METAL)
        constexpr unsigned char test_pattern_msl[] = {
#include "shaders/test_pattern.metal.h"
        };

        constexpr unsigned char canvas_msl[] = {
#include "shaders/canvas.metal.h"
        };

        constexpr unsigned char post_msl[] = {
#include "shaders/post.metal.h"
        };
#endif

        template <std::size_t N> std::span<const std::byte> Bytes(const unsigned char (&data)[N]) noexcept
        {
            return std::as_bytes(std::span(data));
        }
    } // namespace

    std::optional<ShaderBlob> BuiltinShaderBlob(BuiltinShader shader, ShaderTarget target) noexcept
    {
        switch (target) {
            case ShaderTarget::None:
                return ShaderBlob{ .target = ShaderTarget::None, .debug_name = "builtin" };
#if defined(ATMO_RENDERING_VULKAN)
            case ShaderTarget::SpirV:
                switch (shader) {
                    case BuiltinShader::TestPattern:
                        return ShaderBlob{ .target = ShaderTarget::SpirV, .code = Bytes(test_pattern_spirv), .debug_name = "test_pattern" };
                    case BuiltinShader::Canvas:
                        return ShaderBlob{ .target = ShaderTarget::SpirV, .code = Bytes(canvas_spirv), .debug_name = "canvas" };
                    case BuiltinShader::Post:
                        return ShaderBlob{ .target = ShaderTarget::SpirV, .code = Bytes(post_spirv), .debug_name = "post" };
                }
                break;
#endif
#if defined(ATMO_RENDERING_METAL)
            case ShaderTarget::Msl:
                switch (shader) {
                    case BuiltinShader::TestPattern:
                        return ShaderBlob{ .target = ShaderTarget::Msl, .code = Bytes(test_pattern_msl), .debug_name = "test_pattern" };
                    case BuiltinShader::Canvas:
                        return ShaderBlob{ .target = ShaderTarget::Msl, .code = Bytes(canvas_msl), .debug_name = "canvas" };
                    case BuiltinShader::Post:
                        return ShaderBlob{ .target = ShaderTarget::Msl, .code = Bytes(post_msl), .debug_name = "post" };
                }
                break;
#endif
            default:
                break;
        }
        return std::nullopt;
    }
} // namespace atmo::server::rendering
