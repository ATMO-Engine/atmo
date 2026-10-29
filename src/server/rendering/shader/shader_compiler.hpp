#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "server/rendering/material_types.hpp"
#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    struct ShaderEntryPoint {
        std::string name;
        ShaderStage stage = ShaderStage::Vertex;
    };

    enum class ShaderBindingKind : std::uint8_t {
        Texture,
        Sampler,
        ConstantBuffer,
        StorageBuffer,
        Other,
    };

    struct ShaderBinding {
        std::string name;
        std::uint32_t set = 0;
        std::uint32_t binding = 0;
        ShaderBindingKind kind = ShaderBindingKind::Other;
        std::uint32_t count = 1;
    };

    struct ShaderLayout {
        std::vector<ShaderEntryPoint> entry_points;
        std::vector<ShaderBinding> bindings;
        std::uint32_t push_constant_size = 0;
    };

    struct CompiledShader {
        ShaderTarget target = ShaderTarget::None;
        std::vector<std::byte> code;
        ShaderLayout layout;
        std::string diagnostics;

        [[nodiscard]] ShaderBlob blob() const noexcept;
    };

    struct ShaderSource {
        std::string name;
        std::string path;
        std::string code;
        std::vector<std::string> search_paths;
    };

    struct CompiledMaterial {
        std::string type_name;
        ShaderDesc desc;
        std::string diagnostics;
    };

    class ShaderCompiler
    {
    public:
        ShaderCompiler();
        ~ShaderCompiler();

        ShaderCompiler(const ShaderCompiler &) = delete;
        ShaderCompiler &operator=(const ShaderCompiler &) = delete;

        [[nodiscard]] static bool Supports(ShaderTarget target) noexcept;

        [[nodiscard]] std::expected<CompiledShader, std::string> compile(const ShaderSource &source, ShaderTarget target);
        [[nodiscard]] std::expected<CompiledMaterial, std::string> compileMaterial(const ShaderSource &source, std::span<const ShaderTarget> targets);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace atmo::server::rendering
