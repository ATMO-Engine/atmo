#pragma once

#include <array>
#include <span>
#include <utility>
#include <vector>

#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    class TestPattern
    {
    public:
        explicit TestPattern(RenderingPlatform &platform);
        ~TestPattern();

        TestPattern(const TestPattern &) = delete;
        TestPattern &operator=(const TestPattern &) = delete;

        void render(std::span<const SwapchainHandle> swapchains);
        [[nodiscard]] bool isAvailable() const noexcept;

        static constexpr std::array<float, 4> clear_color{ 0.08f, 0.09f, 0.11f, 1.0f };
        static constexpr std::array<std::uint8_t, 4> light_texel{ 0xF2, 0xA6, 0x3B, 0xFF };
        static constexpr std::array<std::uint8_t, 4> dark_texel{ 0x2E, 0x6F, 0xB5, 0xFF };
        static constexpr int texture_size = 64;
        static constexpr int checker_size = 8;
        static constexpr float quad_scale = 0.6f;

    private:
        bool ensureResources();
        PipelineHandle pipelineFor(TextureFormat format);
        void release() noexcept;

        RenderingPlatform &m_platform;
        ShaderModuleHandle m_module;
        TextureHandle m_texture;
        SamplerHandle m_sampler;
        BufferHandle m_indices;
        std::vector<std::pair<TextureFormat, PipelineHandle>> m_pipelines;
        bool m_ready = false;
        bool m_failed = false;
    };
} // namespace atmo::server::rendering
