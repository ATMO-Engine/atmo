#pragma once

#include <functional>
#include <string>
#include <vector>

#include "server/rendering/platform/rendering_platform.hpp"

namespace atmo::server::rendering
{
    class RenderGraph
    {
    public:
        struct PassDesc {
            std::string name;
            std::vector<ColorAttachment> color_attachments;
            std::vector<TextureHandle> sampled;
        };

        using RecordFunction = std::function<void(CommandList &)>;

        void addPass(PassDesc desc, RecordFunction record);
        void addPresent(TextureHandle image);

        void execute(CommandList &list);
        void reset();

        [[nodiscard]] std::size_t passCount() const noexcept;
        [[nodiscard]] const std::vector<std::size_t> &executionOrder() const noexcept;

    private:
        struct Pass {
            PassDesc desc;
            RecordFunction record;
        };

        void sortPasses();

        std::vector<Pass> m_passes;
        std::vector<TextureHandle> m_present;
        std::vector<std::size_t> m_order;
    };
} // namespace atmo::server::rendering
