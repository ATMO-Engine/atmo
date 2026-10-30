#include "null_platform.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>

namespace atmo::server::rendering
{
    void NullCommandList::barrier(std::span<const TextureBarrier> textures)
    {
        if (m_in_render_pass)
            throw std::logic_error("barrier called inside a render pass");
    }

    void NullCommandList::renderPassBegin(const RenderPassDesc &desc)
    {
        if (m_in_render_pass)
            throw std::logic_error("renderPassBegin called inside a render pass");
        if (desc.color_attachments.empty())
            throw std::invalid_argument("renderPassBegin needs at least one color attachment");
        m_in_render_pass = true;
        ++m_render_passes;
    }

    void NullCommandList::renderPassEnd()
    {
        if (!m_in_render_pass)
            throw std::logic_error("renderPassEnd called outside a render pass");
        m_in_render_pass = false;
        m_pipeline_bound = false;
    }

    void NullCommandList::pipelineBind(PipelineHandle pipeline)
    {
        requireRenderPass("pipelineBind");
        if (!pipeline.isValid())
            throw std::invalid_argument("pipelineBind: invalid pipeline");
        m_pipeline_bound = true;
    }

    void NullCommandList::constantsPush(std::span<const std::byte> data)
    {
        if (data.size() > 128 || data.size() % 4 != 0)
            throw std::invalid_argument("constantsPush: size must be a multiple of 4 and at most 128 bytes");
    }

    void NullCommandList::indexBufferBind(BufferHandle buffer, IndexType type, std::uint64_t offset)
    {
        if (!buffer.isValid())
            throw std::invalid_argument("indexBufferBind: invalid buffer");
    }

    void NullCommandList::viewportSet(const Viewport &viewport)
    {
        requireRenderPass("viewportSet");
    }

    void NullCommandList::scissorSet(const Region2i &scissor)
    {
        requireRenderPass("scissorSet");
    }

    void NullCommandList::draw(std::uint32_t vertices, std::uint32_t instances, std::uint32_t first_vertex, std::uint32_t first_instance)
    {
        requireRenderPass("draw");
        if (!m_pipeline_bound)
            throw std::logic_error("draw called without a bound pipeline");
        ++m_draws;
    }

    void NullCommandList::drawIndexed(
        std::uint32_t indices, std::uint32_t instances, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance)
    {
        requireRenderPass("drawIndexed");
        if (!m_pipeline_bound)
            throw std::logic_error("drawIndexed called without a bound pipeline");
        ++m_draws;
    }

    void NullCommandList::debugLabelPush(std::string_view label)
    {
        ++m_label_depth;
    }

    void NullCommandList::debugLabelPop()
    {
        if (m_label_depth == 0)
            throw std::logic_error("debugLabelPop called without a matching push");
        --m_label_depth;
    }

    void NullCommandList::reset()
    {
        m_draws = 0;
        m_render_passes = 0;
        m_label_depth = 0;
        m_in_render_pass = false;
        m_pipeline_bound = false;
    }

    bool NullCommandList::isIdle() const noexcept
    {
        return !m_in_render_pass && m_label_depth == 0;
    }

    std::uint64_t NullCommandList::drawCount() const noexcept
    {
        return m_draws;
    }

    std::uint64_t NullCommandList::renderPassCount() const noexcept
    {
        return m_render_passes;
    }

    void NullCommandList::requireRenderPass(const char *call) const
    {
        if (!m_in_render_pass)
            throw std::logic_error(std::format("{} called outside a render pass", call));
    }

    NullPlatform::NullPlatform()
    {
        m_caps = PlatformCaps{
            .device_name = "Null",
            .max_texture_size = 16384,
            .max_bindless_textures = 65536,
            .max_bindless_samplers = 16,
            .max_bindless_buffers = 4096,
            .max_push_constant_size = 128,
            .frames_in_flight = 1,
            .presentation = false,
        };
    }

    PlatformKind NullPlatform::kind() const noexcept
    {
        return PlatformKind::Null;
    }

    std::string_view NullPlatform::name() const noexcept
    {
        return "Null";
    }

    bool NullPlatform::initialize(const RenderingSettings &settings)
    {
        return true;
    }

    void NullPlatform::shutdown()
    {
        m_swapchains.clear();
        m_buffers.clear();
        m_textures.clear();
        m_samplers.clear();
        m_modules.clear();
        m_pipelines.clear();
        m_in_frame = false;
    }

    SDL_WindowFlags NullPlatform::windowFlags() const noexcept
    {
        return 0;
    }

    const PlatformCaps &NullPlatform::caps() const noexcept
    {
        return m_caps;
    }

    ShaderTarget NullPlatform::shaderTarget() const noexcept
    {
        return ShaderTarget::None;
    }

    SwapchainHandle NullPlatform::swapchainCreate(const NativeWindow &window, const SwapchainDesc &desc)
    {
        TextureHandle image = m_textures.insert(
            { .desc = { .size = desc.size, .format = TextureFormat::Bgra8Unorm, .usage = TextureUsage::RenderTarget }, .swapchain_image = true });
        return m_swapchains.insert({ .size = desc.size, .image = image });
    }

    void NullPlatform::swapchainResize(SwapchainHandle swapchain, core::types::Vector2i size)
    {
        Swapchain *entry = m_swapchains.get(swapchain);
        if (!entry)
            throw std::invalid_argument("swapchainResize: unknown swapchain");
        entry->size = size;
        m_textures.at(entry->image).desc.size = size;
    }

    void NullPlatform::swapchainDestroy(SwapchainHandle swapchain)
    {
        std::optional<Swapchain> entry = m_swapchains.remove(swapchain);
        if (!entry)
            throw std::invalid_argument("swapchainDestroy: unknown swapchain");
        m_textures.remove(entry->image);
    }

    TextureFormat NullPlatform::swapchainFormat(SwapchainHandle swapchain) const
    {
        if (!m_swapchains.get(swapchain))
            throw std::invalid_argument("swapchainFormat: unknown swapchain");
        return TextureFormat::Bgra8Unorm;
    }

    TextureHandle NullPlatform::swapchainAcquire(SwapchainHandle swapchain)
    {
        requireFrame("swapchainAcquire");
        Swapchain &entry = m_swapchains.at(swapchain);
        if (entry.acquired)
            throw std::logic_error("swapchainAcquire: swapchain already acquired this frame");
        if (entry.size.x <= 0 || entry.size.y <= 0)
            return {};
        entry.acquired = true;
        return entry.image;
    }

    void NullPlatform::swapchainPresent(SwapchainHandle swapchain)
    {
        requireFrame("swapchainPresent");
        Swapchain &entry = m_swapchains.at(swapchain);
        if (!entry.acquired)
            throw std::logic_error("swapchainPresent: swapchain was not acquired");
        if (!m_submitted)
            throw std::logic_error("swapchainPresent: nothing was submitted this frame");
        entry.acquired = false;
        ++m_present_count;
    }

    BufferHandle NullPlatform::bufferCreate(const BufferDesc &desc)
    {
        if (desc.size == 0)
            throw std::invalid_argument("bufferCreate: size must not be 0");
        Buffer buffer{ .desc = desc };
        if (desc.memory != MemoryType::GpuOnly)
            buffer.data.resize(desc.size);
        return m_buffers.insert(std::move(buffer));
    }

    void NullPlatform::bufferWrite(BufferHandle buffer, std::uint64_t offset, std::span<const std::byte> data)
    {
        Buffer &entry = m_buffers.at(buffer);
        if (entry.desc.memory != MemoryType::Upload)
            throw std::logic_error("bufferWrite: buffer is not host visible");
        if (offset + data.size() > entry.desc.size)
            throw std::out_of_range("bufferWrite: write past the end of the buffer");
        std::memcpy(entry.data.data() + offset, data.data(), data.size());
    }

    TextureHandle NullPlatform::textureCreate(const TextureDesc &desc)
    {
        if (desc.size.x <= 0 || desc.size.y <= 0 || static_cast<std::uint32_t>(std::max(desc.size.x, desc.size.y)) > m_caps.max_texture_size)
            throw std::invalid_argument("textureCreate: invalid size");
        if (BytesPerPixel(desc.format) == 0)
            throw std::invalid_argument("textureCreate: invalid format");
        return m_textures.insert({ .desc = desc });
    }

    void NullPlatform::textureUpload(TextureHandle texture, const Region2i &region, std::span<const std::byte> pixels)
    {
        Texture &entry = m_textures.at(texture);
        if (!HasFlag(entry.desc.usage, TextureUsage::CopyDst))
            throw std::logic_error("textureUpload: texture was not created with CopyDst usage");
        if (region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 || region.x + region.width > entry.desc.size.x ||
            region.y + region.height > entry.desc.size.y)
            throw std::out_of_range("textureUpload: region outside the texture");

        const std::size_t bpp = BytesPerPixel(entry.desc.format);
        const std::size_t row = static_cast<std::size_t>(region.width) * bpp;
        if (pixels.size() != row * static_cast<std::size_t>(region.height))
            throw std::invalid_argument("textureUpload: pixel data does not match the region");

        const std::size_t pitch = static_cast<std::size_t>(entry.desc.size.x) * bpp;
        if (entry.pixels.empty())
            entry.pixels.resize(pitch * static_cast<std::size_t>(entry.desc.size.y));
        for (std::int32_t y = 0; y < region.height; ++y)
            std::memcpy(
                entry.pixels.data() + (static_cast<std::size_t>(region.y + y) * pitch) + (static_cast<std::size_t>(region.x) * bpp),
                pixels.data() + (static_cast<std::size_t>(y) * row),
                row);
    }

    std::vector<std::byte> NullPlatform::textureReadback(TextureHandle texture)
    {
        if (m_in_frame)
            throw std::logic_error("textureReadback called during a frame");
        const Texture &entry = m_textures.at(texture);
        if (!entry.pixels.empty())
            return entry.pixels;
        return std::vector<std::byte>(
            static_cast<std::size_t>(entry.desc.size.x) * static_cast<std::size_t>(entry.desc.size.y) * BytesPerPixel(entry.desc.format));
    }

    core::types::Vector2i NullPlatform::textureSize(TextureHandle texture) const
    {
        return m_textures.at(texture).desc.size;
    }

    SamplerHandle NullPlatform::samplerCreate(const SamplerDesc &desc)
    {
        if (m_samplers.size() >= m_caps.max_bindless_samplers)
            throw std::length_error("samplerCreate: bindless sampler table is full");
        return m_samplers.insert(desc);
    }

    ShaderModuleHandle NullPlatform::shaderModuleCreate(const ShaderBlob &blob)
    {
        return m_modules.insert(blob.target);
    }

    PipelineHandle NullPlatform::graphicsPipelineCreate(const GraphicsPipelineDesc &desc)
    {
        if (!m_modules.get(desc.vertex.module) || !m_modules.get(desc.fragment.module))
            throw std::invalid_argument("graphicsPipelineCreate: unknown shader module");
        if (desc.color_format == TextureFormat::Undefined)
            throw std::invalid_argument("graphicsPipelineCreate: color format is undefined");
        return m_pipelines.insert(desc);
    }

    void NullPlatform::destroy(BufferHandle buffer)
    {
        if (!m_buffers.remove(buffer))
            throw std::invalid_argument("destroy: unknown buffer");
    }

    void NullPlatform::destroy(TextureHandle texture)
    {
        const Texture *entry = m_textures.get(texture);
        if (!entry || entry->swapchain_image)
            throw std::invalid_argument("destroy: unknown texture");
        m_textures.remove(texture);
    }

    void NullPlatform::destroy(SamplerHandle sampler)
    {
        if (!m_samplers.remove(sampler))
            throw std::invalid_argument("destroy: unknown sampler");
    }

    void NullPlatform::destroy(ShaderModuleHandle module)
    {
        if (!m_modules.remove(module))
            throw std::invalid_argument("destroy: unknown shader module");
    }

    void NullPlatform::destroy(PipelineHandle pipeline)
    {
        if (!m_pipelines.remove(pipeline))
            throw std::invalid_argument("destroy: unknown pipeline");
    }

    std::uint32_t NullPlatform::bindlessIndex(TextureHandle texture) const
    {
        if (!HasFlag(m_textures.at(texture).desc.usage, TextureUsage::Sampled))
            throw std::logic_error("bindlessIndex: texture was not created with Sampled usage");
        return texture.value & HandlePool<TextureHandle, Texture>::slot_mask;
    }

    std::uint32_t NullPlatform::bindlessIndex(SamplerHandle sampler) const
    {
        static_cast<void>(m_samplers.at(sampler));
        return sampler.value & HandlePool<SamplerHandle, SamplerDesc>::slot_mask;
    }

    std::uint32_t NullPlatform::bindlessIndex(BufferHandle buffer) const
    {
        if (!HasFlag(m_buffers.at(buffer).desc.usage, BufferUsage::Storage))
            throw std::logic_error("bindlessIndex: buffer was not created with Storage usage");
        return buffer.value & HandlePool<BufferHandle, Buffer>::slot_mask;
    }

    void NullPlatform::frameBegin()
    {
        if (m_in_frame)
            throw std::logic_error("frameBegin called twice without frameEnd");
        m_in_frame = true;
        m_submitted = false;
        m_command_list.reset();
    }

    CommandList &NullPlatform::commandListAcquire(QueueType queue)
    {
        requireFrame("commandListAcquire");
        return m_command_list;
    }

    void NullPlatform::submit(std::span<CommandList *const> lists)
    {
        requireFrame("submit");
        for (CommandList *list : lists) {
            auto *null_list = dynamic_cast<NullCommandList *>(list);
            if (!null_list || !null_list->isIdle())
                throw std::logic_error("submit: command list has an open render pass or debug label");
            m_draw_count += null_list->drawCount();
            null_list->reset();
        }
        m_submitted = true;
    }

    void NullPlatform::frameEnd()
    {
        requireFrame("frameEnd");
        if (!m_command_list.isIdle()) {
            m_in_frame = false;
            m_command_list.reset();
            throw std::logic_error("frameEnd: command list has an open render pass or debug label");
        }
        m_submitted = true;
        m_swapchains.forEach([this](SwapchainHandle, Swapchain &swapchain) {
            if (swapchain.acquired) {
                swapchain.acquired = false;
                ++m_present_count;
            }
        });
        m_in_frame = false;
        ++m_frame_count;
    }

    std::size_t NullPlatform::swapchainCount() const noexcept
    {
        return m_swapchains.size();
    }

    std::optional<core::types::Vector2i> NullPlatform::swapchainSize(SwapchainHandle swapchain) const
    {
        const Swapchain *entry = m_swapchains.get(swapchain);
        if (!entry)
            return std::nullopt;
        return entry->size;
    }

    std::vector<core::types::Vector2i> NullPlatform::swapchainSizes()
    {
        std::vector<core::types::Vector2i> sizes;
        m_swapchains.forEach([&sizes](SwapchainHandle, const Swapchain &swapchain) { sizes.push_back(swapchain.size); });
        return sizes;
    }

    std::uint64_t NullPlatform::frameCount() const noexcept
    {
        return m_frame_count;
    }

    std::uint64_t NullPlatform::presentCount() const noexcept
    {
        return m_present_count;
    }

    std::uint64_t NullPlatform::drawCount() const noexcept
    {
        return m_draw_count;
    }

    std::size_t NullPlatform::liveObjectCount() const noexcept
    {
        return m_buffers.size() + (m_textures.size() - m_swapchains.size()) + m_samplers.size() + m_modules.size() + m_pipelines.size();
    }

    void NullPlatform::requireFrame(const char *call) const
    {
        if (!m_in_frame)
            throw std::logic_error(std::format("{} called outside frameBegin/frameEnd", call));
    }
} // namespace atmo::server::rendering
