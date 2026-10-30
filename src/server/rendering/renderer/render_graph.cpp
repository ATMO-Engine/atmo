#include "render_graph.hpp"

#include <algorithm>
#include <stdexcept>

namespace atmo::server::rendering
{
    void RenderGraph::addPass(PassDesc desc, RecordFunction record)
    {
        if (desc.color_attachments.empty())
            throw std::invalid_argument("RenderGraph pass needs at least one color attachment");
        m_passes.push_back({ std::move(desc), std::move(record) });
    }

    void RenderGraph::addPresent(TextureHandle image)
    {
        m_present.push_back(image);
    }

    void RenderGraph::reset()
    {
        m_passes.clear();
        m_present.clear();
        m_order.clear();
    }

    std::size_t RenderGraph::passCount() const noexcept
    {
        return m_passes.size();
    }

    const std::vector<std::size_t> &RenderGraph::executionOrder() const noexcept
    {
        return m_order;
    }

    void RenderGraph::sortPasses()
    {
        const std::size_t count = m_passes.size();
        std::vector<std::vector<std::size_t>> dependents(count);
        std::vector<std::size_t> pending(count, 0);

        auto writes = [this](std::size_t pass, TextureHandle texture) {
            return std::ranges::any_of(m_passes[pass].desc.color_attachments, [texture](const ColorAttachment &a) { return a.texture == texture; });
        };

        for (std::size_t reader = 0; reader < count; ++reader) {
            for (TextureHandle texture : m_passes[reader].desc.sampled) {
                for (std::size_t writer = 0; writer < count; ++writer) {
                    if (writer == reader || !writes(writer, texture))
                        continue;
                    dependents[writer].push_back(reader);
                    ++pending[reader];
                }
            }
        }

        m_order.clear();
        std::vector<bool> done(count, false);
        while (m_order.size() < count) {
            std::size_t pass = count;
            for (std::size_t i = 0; i < count && pass == count; ++i)
                if (!done[i] && pending[i] == 0)
                    pass = i;
            if (pass == count)
                throw std::logic_error("RenderGraph has a dependency cycle");

            done[pass] = true;
            m_order.push_back(pass);
            for (std::size_t dependent : dependents[pass]) --pending[dependent];
        }
    }

    void RenderGraph::execute(CommandList &list)
    {
        sortPasses();

        std::vector<TextureBarrier> barriers;
        for (std::size_t index : m_order) {
            Pass &pass = m_passes[index];

            barriers.clear();
            for (TextureHandle texture : pass.desc.sampled) barriers.push_back({ .texture = texture, .after = ResourceState::ShaderRead });
            for (const ColorAttachment &attachment : pass.desc.color_attachments)
                barriers.push_back({ .texture = attachment.texture, .after = ResourceState::RenderTarget });

            list.debugLabelPush(pass.desc.name);
            list.barrier(barriers);
            list.renderPassBegin({ .color_attachments = pass.desc.color_attachments, .debug_name = pass.desc.name });
            pass.record(list);
            list.renderPassEnd();
            list.debugLabelPop();
        }

        barriers.clear();
        for (TextureHandle image : m_present) barriers.push_back({ .texture = image, .after = ResourceState::Present });
        if (!barriers.empty())
            list.barrier(barriers);
    }
} // namespace atmo::server::rendering
