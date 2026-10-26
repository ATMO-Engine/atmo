#include "metal_platform.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <system_error>

#include "metal_utils.hpp"
#include "spdlog/spdlog.h"

namespace atmo::server::rendering
{
    using metal::MetalError;
    using metal::ToNsString;

    namespace
    {
        constexpr std::size_t bindless_entry_size = sizeof(std::uint64_t);
        constexpr std::size_t texture_table_offset = 0;
        constexpr std::size_t sampler_table_offset = texture_table_offset + (MetalPlatform::max_bindless_textures * bindless_entry_size);
        constexpr std::size_t buffer_table_offset = sampler_table_offset + (MetalPlatform::max_bindless_samplers * bindless_entry_size);
        constexpr std::size_t bindless_table_size = buffer_table_offset + (MetalPlatform::max_bindless_buffers * bindless_entry_size);
        constexpr const char *archive_file = "metal-pipelines.bin";

        std::size_t AlignUp(std::size_t value, std::size_t alignment)
        {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        class Pool
        {
        public:
            Pool() : m_pool(NS::AutoreleasePool::alloc()->init()) {}
            ~Pool()
            {
                m_pool->release();
            }

            Pool(const Pool &) = delete;
            Pool &operator=(const Pool &) = delete;

        private:
            NS::AutoreleasePool *m_pool;
        };

        bool ResidencySetsAvailable() noexcept
        {
            const char *setting = std::getenv("ATMO_METAL_RESIDENCY_SETS");
            if (setting && std::strcmp(setting, "0") == 0)
                return false;
            if (__builtin_available(macOS 15.0, *))
                return true;
            return false;
        }
    } // namespace

    MetalPlatform::MetalPlatform() = default;

    MetalPlatform::~MetalPlatform()
    {
        shutdown();
    }

    PlatformKind MetalPlatform::kind() const noexcept
    {
        return PlatformKind::Metal;
    }

    std::string_view MetalPlatform::name() const noexcept
    {
        return "Metal";
    }

    bool MetalPlatform::initialize(const RenderingSettings &settings)
    {
        if (m_initialized)
            return true;

        Pool pool;
        if (!createDevice(settings))
            return false;

        m_queue = NS::TransferPtr(m_device->newCommandQueue());
        if (!m_queue) {
            spdlog::error("Failed to create a Metal command queue");
            return false;
        }
        m_queue->setLabel(ToNsString("Atmo"));

        createBindlessTable();
        createResidency();
        openPipelineArchive(settings);

        for (Frame &frame : m_frames) {
            frame.staging = NS::TransferPtr(m_device->newBuffer(staging_size, MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined));
            if (!frame.staging)
                throw std::runtime_error("failed to allocate the Metal staging buffer");
            frame.staging->setLabel(ToNsString("staging ring"));
        }
        m_command_list = std::make_unique<metal::MetalCommandList>(*this);

        m_caps.device_name = m_device->name() ? m_device->name()->utf8String() : "Metal device";
        m_caps.max_texture_size = 16384;
        m_caps.max_bindless_textures = max_bindless_textures;
        m_caps.max_bindless_samplers = max_bindless_samplers;
        m_caps.max_bindless_buffers = max_bindless_buffers;
        m_caps.max_push_constant_size = push_constant_size;
        m_caps.frames_in_flight = frames_in_flight;
        m_caps.presentation = true;
        m_caps.software_device = false;
        m_initialized = true;

        spdlog::info("Metal on {}{}{}", m_caps.device_name, m_residency ? "" : " (without residency sets)", m_validation ? ", validation enabled" : "");
        return true;
    }

    bool MetalPlatform::createDevice(const RenderingSettings &settings)
    {
        if (settings.gpu_validation) {
            setenv("MTL_DEBUG_LAYER", "1", 0);
            setenv("MTL_SHADER_VALIDATION", "1", 0);
        }
        const char *debug_layer = std::getenv("MTL_DEBUG_LAYER");
        m_validation = debug_layer && std::strcmp(debug_layer, "0") != 0;

        m_device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
        if (!m_device) {
            spdlog::error("No Metal device is available");
            return false;
        }

        const std::string device_name = m_device->name() ? m_device->name()->utf8String() : "the Metal device";
        if (!m_device->supportsFamily(MTL::GPUFamilyMetal3)) {
            spdlog::error("{} does not support Metal 3", device_name);
            m_device.reset();
            return false;
        }
        if (m_device->argumentBuffersSupport() != MTL::ArgumentBuffersTier2) {
            spdlog::error("{} does not support tier 2 argument buffers", device_name);
            m_device.reset();
            return false;
        }
        return true;
    }

    void MetalPlatform::createBindlessTable()
    {
        m_bindless = NS::TransferPtr(m_device->newBuffer(bindless_table_size, MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined));
        if (!m_bindless)
            throw std::runtime_error("failed to allocate the Metal bindless table");
        m_bindless->setLabel(ToNsString("bindless table"));
        std::memset(m_bindless->contents(), 0, bindless_table_size);

        m_free_texture_indices.clear();
        for (std::uint32_t i = max_bindless_textures; i > 0; --i) m_free_texture_indices.push_back(i - 1);
        m_free_sampler_indices.clear();
        for (std::uint32_t i = max_bindless_samplers; i > 0; --i) m_free_sampler_indices.push_back(i - 1);
        m_free_buffer_indices.clear();
        for (std::uint32_t i = max_bindless_buffers; i > 0; --i) m_free_buffer_indices.push_back(i - 1);
    }

    void MetalPlatform::createResidency()
    {
        if (!ResidencySetsAvailable())
            return;

        NS::SharedPtr<MTL::ResidencySetDescriptor> descriptor = NS::TransferPtr(MTL::ResidencySetDescriptor::alloc()->init());
        descriptor->setLabel(ToNsString("bindless resources"));
        descriptor->setInitialCapacity(256);
        NS::Error *error = nullptr;
        m_residency = NS::TransferPtr(m_device->newResidencySet(descriptor.get(), &error));
        if (!m_residency) {
            spdlog::warn("Failed to create a Metal residency set, binding resources per pass instead: {}", metal::ErrorString(error));
            return;
        }
        m_queue->addResidencySet(m_residency.get());
    }

    void MetalPlatform::openPipelineArchive(const RenderingSettings &settings)
    {
        if (settings.pipeline_cache_dir.empty())
            return;

        std::error_code error_code;
        std::filesystem::create_directories(settings.pipeline_cache_dir, error_code);
        m_archive_path = std::filesystem::path(settings.pipeline_cache_dir) / archive_file;

        NS::SharedPtr<MTL::BinaryArchiveDescriptor> descriptor = NS::TransferPtr(MTL::BinaryArchiveDescriptor::alloc()->init());
        NS::Error *error = nullptr;
        if (std::filesystem::exists(m_archive_path, error_code)) {
            descriptor->setUrl(NS::URL::fileURLWithPath(ToNsString(m_archive_path.string())));
            m_archive = NS::TransferPtr(m_device->newBinaryArchive(descriptor.get(), &error));
            if (!m_archive)
                spdlog::warn("Discarding the Metal pipeline cache {}: {}", m_archive_path.string(), metal::ErrorString(error));
        }
        if (!m_archive) {
            descriptor->setUrl(nullptr);
            error = nullptr;
            m_archive = NS::TransferPtr(m_device->newBinaryArchive(descriptor.get(), &error));
            if (!m_archive)
                spdlog::warn("Failed to create a Metal pipeline cache: {}", metal::ErrorString(error));
        }
    }

    void MetalPlatform::savePipelineArchive()
    {
        if (!m_archive || !m_archive_dirty || m_archive_path.empty())
            return;

        Pool pool;
        NS::Error *error = nullptr;
        if (!m_archive->serializeToURL(NS::URL::fileURLWithPath(ToNsString(m_archive_path.string())), &error))
            spdlog::warn("Failed to save the Metal pipeline cache {}: {}", m_archive_path.string(), metal::ErrorString(error));
        m_archive_dirty = false;
    }

    void MetalPlatform::shutdown()
    {
        if (m_device) {
            Pool pool;
            if (m_in_frame) {
                try {
                    frameEnd();
                } catch (const std::exception &e) {
                    spdlog::error("Failed to end the Metal frame during shutdown: {}", e.what());
                }
            }
            for (Frame &frame : m_frames) flushUploads(frame);
            waitIdle();
            savePipelineArchive();

            const std::size_t leaked = m_pipelines.size() + m_modules.size() + m_samplers.size() + m_textures.size() + m_buffers.size();
            if (leaked > m_swapchains.size())
                spdlog::debug("Destroying {} Metal objects that were never freed", leaked - m_swapchains.size());

            collectGarbage(~0ull);
            m_swapchains.clear();
            m_pipelines.clear();
            m_modules.clear();
            m_samplers.clear();
            m_textures.clear();
            m_buffers.clear();
            m_command_list.reset();
            m_upload_encoder.reset();
            for (Frame &frame : m_frames) frame = Frame{};
            m_resident.clear();
            m_written.clear();
            m_residency.reset();
            m_archive.reset();
            m_bindless.reset();
            m_queue.reset();
            m_device.reset();
        }

        m_deletions.clear();
        m_archive_path.clear();
        m_archive_dirty = false;
        m_residency_dirty = false;
        m_frame_number = 0;
        m_completed_frame = 0;
        m_in_frame = false;
        m_submitted = false;
        m_validation = false;
        m_initialized = false;
    }

    SDL_WindowFlags MetalPlatform::windowFlags() const noexcept
    {
        return 0;
    }

    const PlatformCaps &MetalPlatform::caps() const noexcept
    {
        return m_caps;
    }

    ShaderTarget MetalPlatform::shaderTarget() const noexcept
    {
        return ShaderTarget::Msl;
    }

    MetalPlatform::Frame &MetalPlatform::prepareFrame(std::uint64_t number)
    {
        Frame &frame = m_frames[number % frames_in_flight];
        if (frame.number == number)
            return frame;

        retire(frame.uploads);
        retire(frame.commands);
        frame.uploads.reset();
        frame.commands.reset();

        if (number > frames_in_flight)
            m_completed_frame = std::max(m_completed_frame, number - frames_in_flight);
        collectGarbage(m_completed_frame);

        frame.staging_offset = 0;
        frame.number = number;
        return frame;
    }

    MTL::CommandBuffer *MetalPlatform::newCommandBuffer(std::string_view label)
    {
        NS::SharedPtr<MTL::CommandBufferDescriptor> descriptor = NS::TransferPtr(MTL::CommandBufferDescriptor::alloc()->init());
        if (m_validation)
            descriptor->setErrorOptions(MTL::CommandBufferErrorOptionEncoderExecutionStatus);

        MTL::CommandBuffer *commands = m_queue->commandBuffer(descriptor.get());
        if (!commands)
            throw std::runtime_error("failed to create a Metal command buffer");
        commands->setLabel(ToNsString(label));
        return commands;
    }

    void MetalPlatform::retire(NS::SharedPtr<MTL::CommandBuffer> &commands)
    {
        if (!commands)
            return;
        if (commands->status() == MTL::CommandBufferStatusNotEnqueued)
            return;
        commands->waitUntilCompleted();
        if (commands->status() == MTL::CommandBufferStatusError) {
            ++m_gpu_errors;
            spdlog::error("Metal command buffer failed: {}", metal::ErrorString(commands->error()));
        }
        commands.reset();
    }

    MTL::BlitCommandEncoder *MetalPlatform::transferEncoder()
    {
        if (m_in_frame) {
            if (m_submitted)
                throw std::logic_error("GPU transfers are not allowed after the frame was submitted");
            return m_command_list->blitEncoder();
        }

        Frame &frame = prepareFrame(m_frame_number + 1);
        if (!frame.uploads)
            frame.uploads = NS::RetainPtr(newCommandBuffer("uploads"));
        if (!m_upload_encoder)
            m_upload_encoder = NS::RetainPtr(frame.uploads->blitCommandEncoder());
        return m_upload_encoder.get();
    }

    void MetalPlatform::flushUploads(Frame &frame)
    {
        if (m_upload_encoder) {
            m_upload_encoder->endEncoding();
            m_upload_encoder.reset();
        }
        if (frame.uploads && frame.uploads->status() == MTL::CommandBufferStatusNotEnqueued) {
            commitResidency();
            frame.uploads->commit();
        }
    }

    void MetalPlatform::commitResidency()
    {
        if (!m_residency || !m_residency_dirty)
            return;
        m_residency->commit();
        m_residency_dirty = false;
    }

    MetalPlatform::StagingSpan MetalPlatform::stagingAllocate(std::size_t size)
    {
        Frame &frame = m_in_frame ? m_frames[m_frame_number % frames_in_flight] : prepareFrame(m_frame_number + 1);

        const std::size_t offset = AlignUp(frame.staging_offset, 16);
        if (offset + size <= staging_size) {
            frame.staging_offset = offset + size;
            return { frame.staging.get(), offset, static_cast<std::byte *>(frame.staging->contents()) + offset };
        }

        NS::SharedPtr<MTL::Buffer> buffer = NS::TransferPtr(m_device->newBuffer(size, MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined));
        if (!buffer)
            throw std::runtime_error("failed to allocate a Metal staging buffer");
        defer([buffer] {});
        return { buffer.get(), 0, static_cast<std::byte *>(buffer->contents()) };
    }

    void MetalPlatform::defer(std::function<void()> destroy)
    {
        m_deletions.push_back({ .frame = m_in_frame ? m_frame_number : m_frame_number + 1, .destroy = std::move(destroy) });
    }

    void MetalPlatform::collectGarbage(std::uint64_t completed_frame) noexcept
    {
        auto ready = [completed_frame](const Deletion &deletion) { return deletion.frame <= completed_frame; };
        std::vector<Deletion> remaining;
        std::vector<Deletion> due;
        for (Deletion &deletion : m_deletions) (ready(deletion) ? due : remaining).push_back(std::move(deletion));
        m_deletions = std::move(remaining);
        for (Deletion &deletion : due) deletion.destroy();
        std::erase_if(m_written, [completed_frame](const auto &entry) { return entry.second <= completed_frame; });
    }

    void MetalPlatform::waitIdle() noexcept
    {
        for (Frame &frame : m_frames) {
            retire(frame.uploads);
            retire(frame.commands);
        }
        m_completed_frame = std::max(m_completed_frame, m_in_frame ? m_frame_number - 1 : m_frame_number);
        collectGarbage(m_completed_frame);
    }

    void MetalPlatform::transition(Texture &entry, TextureHandle handle, ResourceState after)
    {
        if (after == ResourceState::RenderTarget || after == ResourceState::CopyDst)
            m_written[handle.value] = m_in_frame ? m_frame_number : m_frame_number + 1;
        entry.state = after;
    }

    void MetalPlatform::makeResident(MTL::Allocation *allocation)
    {
        if (m_residency)
            m_residency->addAllocation(allocation);
        m_residency_dirty = true;
    }

    void MetalPlatform::evict(MTL::Allocation *allocation)
    {
        if (m_residency)
            m_residency->removeAllocation(allocation);
        m_residency_dirty = true;
    }

    void MetalPlatform::writeBindless(std::size_t offset, std::uint64_t value) noexcept
    {
        std::memcpy(static_cast<std::byte *>(m_bindless->contents()) + offset, &value, sizeof(value));
    }

    void MetalPlatform::prepareEncoderResources(std::vector<const MTL::Resource *> &resources, std::span<const TextureHandle> attachments)
    {
        resources.clear();
        auto attached = [attachments](TextureHandle handle) { return std::ranges::find(attachments, handle) != attachments.end(); };

        if (m_residency) {
            for (const auto &[raw, frame] : m_written) {
                const TextureHandle handle{ raw };
                const Texture *entry = m_textures.get(handle);
                if (entry && entry->texture && entry->bindless != invalid_index && !attached(handle))
                    resources.push_back(entry->texture.get());
            }
            return;
        }

        if (m_residency_dirty) {
            m_resident.clear();
            m_textures.forEach([this](TextureHandle, Texture &entry) {
                if (entry.texture && entry.bindless != invalid_index)
                    m_resident.push_back(entry.texture.get());
            });
            m_buffers.forEach([this](BufferHandle, Buffer &entry) {
                if (entry.bindless != invalid_index)
                    m_resident.push_back(entry.buffer.get());
            });
            m_residency_dirty = false;
        }

        resources = m_resident;
        for (TextureHandle handle : attachments) {
            const Texture *entry = m_textures.get(handle);
            if (entry && entry->texture)
                std::erase(resources, static_cast<const MTL::Resource *>(entry->texture.get()));
        }
    }

    MetalPlatform::Texture &MetalPlatform::texture(TextureHandle handle)
    {
        return m_textures.at(handle);
    }

    const MetalPlatform::Texture &MetalPlatform::texture(TextureHandle handle) const
    {
        return m_textures.at(handle);
    }

    SwapchainHandle MetalPlatform::swapchainCreate(const NativeWindow &window, const SwapchainDesc &desc)
    {
        Pool pool;
        Swapchain swapchain;
        if (window.metal_layer)
            swapchain.layer = NS::RetainPtr(static_cast<CA::MetalLayer *>(window.metal_layer));
        else
            swapchain.layer = NS::RetainPtr(CA::MetalLayer::layer());
        if (!swapchain.layer)
            return {};

        swapchain.layer->setDevice(m_device.get());
        swapchain.layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        swapchain.layer->setFramebufferOnly(true);
        swapchain.layer->setMaximumDrawableCount(3);
        swapchain.layer->setDisplaySyncEnabled(desc.vsync);
        swapchain.size = desc.size;
        if (desc.size.x > 0 && desc.size.y > 0)
            swapchain.layer->setDrawableSize(CGSize{ static_cast<CGFloat>(desc.size.x), static_cast<CGFloat>(desc.size.y) });

        swapchain.image = m_textures.insert(
            Texture{
                .desc = { .size = desc.size, .format = TextureFormat::Bgra8Unorm, .usage = TextureUsage::RenderTarget },
                .swapchain_image = true,
            });
        return m_swapchains.insert(std::move(swapchain));
    }

    void MetalPlatform::swapchainResize(SwapchainHandle handle, core::types::Vector2i size)
    {
        Swapchain &swapchain = m_swapchains.at(handle);
        swapchain.size = size;
        if (size.x > 0 && size.y > 0)
            swapchain.layer->setDrawableSize(CGSize{ static_cast<CGFloat>(size.x), static_cast<CGFloat>(size.y) });
    }

    void MetalPlatform::swapchainDestroy(SwapchainHandle handle)
    {
        std::optional<Swapchain> swapchain = m_swapchains.remove(handle);
        if (!swapchain)
            throw std::invalid_argument("swapchainDestroy: unknown swapchain");
        std::optional<Texture> image = m_textures.remove(swapchain->image);
        defer([swapchain = std::move(*swapchain), image = std::move(image)] {});
    }

    TextureFormat MetalPlatform::swapchainFormat(SwapchainHandle handle) const
    {
        static_cast<void>(m_swapchains.at(handle));
        return TextureFormat::Bgra8Unorm;
    }

    TextureHandle MetalPlatform::swapchainAcquire(SwapchainHandle handle)
    {
        if (!m_in_frame || m_submitted)
            throw std::logic_error("swapchainAcquire called outside a frame or after submit");

        Swapchain &swapchain = m_swapchains.at(handle);
        if (swapchain.acquired)
            return swapchain.image;
        if (swapchain.size.x <= 0 || swapchain.size.y <= 0)
            return {};

        CA::MetalDrawable *drawable = swapchain.layer->nextDrawable();
        if (!drawable)
            return {};

        swapchain.drawable = NS::RetainPtr(drawable);
        Texture &image = texture(swapchain.image);
        image.texture = NS::RetainPtr(drawable->texture());
        image.desc.size = { static_cast<int>(image.texture->width()), static_cast<int>(image.texture->height()) };
        image.state = ResourceState::Undefined;
        swapchain.acquired = true;
        swapchain.presented = false;
        return swapchain.image;
    }

    void MetalPlatform::swapchainPresent(SwapchainHandle handle)
    {
        Swapchain &swapchain = m_swapchains.at(handle);
        if (!swapchain.acquired)
            throw std::logic_error("swapchainPresent called without an acquired image");
        if (!m_submitted)
            throw std::logic_error("swapchainPresent called before the frame was submitted");
        swapchain.presented = true;
    }

    BufferHandle MetalPlatform::bufferCreate(const BufferDesc &desc)
    {
        if (desc.size == 0)
            throw std::invalid_argument("bufferCreate: size must not be 0");
        if (desc.usage == BufferUsage::None)
            throw std::invalid_argument("bufferCreate: no usage given");
        const bool storage = HasFlag(desc.usage, BufferUsage::Storage);
        if (storage && m_free_buffer_indices.empty())
            throw std::length_error("bufferCreate: the bindless buffer table is full");

        Pool pool;
        MTL::ResourceOptions options = MTL::ResourceStorageModePrivate;
        if (desc.memory == MemoryType::Upload)
            options = MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined;
        else if (desc.memory == MemoryType::Readback)
            options = MTL::ResourceStorageModeShared;

        Buffer buffer{ .buffer = NS::TransferPtr(m_device->newBuffer(desc.size, options)), .desc = desc };
        buffer.desc.debug_name = {};
        if (!buffer.buffer)
            throw std::runtime_error("bufferCreate: allocation failed");
        if (!desc.debug_name.empty())
            buffer.buffer->setLabel(ToNsString(desc.debug_name));

        if (storage) {
            buffer.bindless = m_free_buffer_indices.back();
            m_free_buffer_indices.pop_back();
            writeBindless(buffer_table_offset + (buffer.bindless * bindless_entry_size), buffer.buffer->gpuAddress());
            makeResident(buffer.buffer.get());
        }

        return m_buffers.insert(std::move(buffer));
    }

    void MetalPlatform::bufferWrite(BufferHandle buffer, std::uint64_t offset, std::span<const std::byte> data)
    {
        Buffer &entry = m_buffers.at(buffer);
        if (entry.desc.memory != MemoryType::Upload)
            throw std::logic_error("bufferWrite: buffer is not host visible");
        if (offset + data.size() > entry.desc.size)
            throw std::out_of_range("bufferWrite: write past the end of the buffer");
        std::memcpy(static_cast<std::byte *>(entry.buffer->contents()) + offset, data.data(), data.size());
    }

    TextureHandle MetalPlatform::textureCreate(const TextureDesc &desc)
    {
        if (desc.size.x <= 0 || desc.size.y <= 0 || static_cast<std::uint32_t>(std::max(desc.size.x, desc.size.y)) > m_caps.max_texture_size)
            throw std::invalid_argument("textureCreate: invalid size");
        const MTL::PixelFormat format = metal::ToMtlFormat(desc.format);
        if (format == MTL::PixelFormatInvalid)
            throw std::invalid_argument("textureCreate: invalid format");
        if (desc.usage == TextureUsage::None)
            throw std::invalid_argument("textureCreate: no usage given");
        const bool sampled = HasFlag(desc.usage, TextureUsage::Sampled);
        if (sampled && m_free_texture_indices.empty())
            throw std::length_error("textureCreate: the bindless texture table is full");

        Pool pool;
        MTL::TextureDescriptor *descriptor =
            MTL::TextureDescriptor::texture2DDescriptor(format, static_cast<NS::UInteger>(desc.size.x), static_cast<NS::UInteger>(desc.size.y), false);
        descriptor->setUsage(metal::ToMtlTextureUsage(desc.usage));
        descriptor->setStorageMode(MTL::StorageModePrivate);

        Texture texture{ .texture = NS::TransferPtr(m_device->newTexture(descriptor)), .desc = desc };
        texture.desc.debug_name = {};
        if (!texture.texture)
            throw std::runtime_error("textureCreate: allocation failed");
        if (!desc.debug_name.empty())
            texture.texture->setLabel(ToNsString(desc.debug_name));

        if (sampled) {
            texture.bindless = m_free_texture_indices.back();
            m_free_texture_indices.pop_back();
            writeBindless(texture_table_offset + (texture.bindless * bindless_entry_size), texture.texture->gpuResourceID()._impl);
            makeResident(texture.texture.get());
        }

        return m_textures.insert(std::move(texture));
    }

    void MetalPlatform::textureUpload(TextureHandle handle, const Region2i &region, std::span<const std::byte> pixels)
    {
        Texture &entry = texture(handle);
        if (entry.swapchain_image)
            throw std::logic_error("textureUpload: swapchain images cannot be uploaded to");
        if (!HasFlag(entry.desc.usage, TextureUsage::CopyDst))
            throw std::logic_error("textureUpload: texture was not created with CopyDst usage");
        if (region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 || region.x + region.width > entry.desc.size.x ||
            region.y + region.height > entry.desc.size.y)
            throw std::out_of_range("textureUpload: region outside the texture");

        const std::size_t row = static_cast<std::size_t>(region.width) * BytesPerPixel(entry.desc.format);
        const std::size_t size = row * static_cast<std::size_t>(region.height);
        if (pixels.size() != size)
            throw std::invalid_argument("textureUpload: pixel data does not match the region");

        Pool pool;
        MTL::BlitCommandEncoder *encoder = transferEncoder();
        const StagingSpan staging = stagingAllocate(size);
        std::memcpy(staging.data, pixels.data(), pixels.size());

        transition(entry, handle, ResourceState::CopyDst);
        encoder->copyFromBuffer(
            staging.buffer,
            staging.offset,
            row,
            size,
            MTL::Size::Make(static_cast<NS::UInteger>(region.width), static_cast<NS::UInteger>(region.height), 1),
            entry.texture.get(),
            0,
            0,
            MTL::Origin::Make(static_cast<NS::UInteger>(region.x), static_cast<NS::UInteger>(region.y), 0));
        if (HasFlag(entry.desc.usage, TextureUsage::Sampled))
            transition(entry, handle, ResourceState::ShaderRead);
    }

    std::vector<std::byte> MetalPlatform::textureReadback(TextureHandle handle)
    {
        if (m_in_frame)
            throw std::logic_error("textureReadback called during a frame");

        Texture &entry = texture(handle);
        if (entry.swapchain_image)
            throw std::logic_error("textureReadback: swapchain images cannot be read back");
        if (!HasFlag(entry.desc.usage, TextureUsage::CopySrc))
            throw std::logic_error("textureReadback: texture was not created with CopySrc usage");

        Pool pool;
        const std::size_t row = static_cast<std::size_t>(entry.desc.size.x) * BytesPerPixel(entry.desc.format);
        const std::size_t size = row * static_cast<std::size_t>(entry.desc.size.y);
        NS::SharedPtr<MTL::Buffer> buffer = NS::TransferPtr(m_device->newBuffer(size, MTL::ResourceStorageModeShared));
        if (!buffer)
            throw std::runtime_error("textureReadback: allocation failed");

        MTL::BlitCommandEncoder *encoder = transferEncoder();
        encoder->copyFromTexture(
            entry.texture.get(),
            0,
            0,
            MTL::Origin::Make(0, 0, 0),
            MTL::Size::Make(static_cast<NS::UInteger>(entry.desc.size.x), static_cast<NS::UInteger>(entry.desc.size.y), 1),
            buffer.get(),
            0,
            row,
            size);

        Frame &frame = m_frames[(m_frame_number + 1) % frames_in_flight];
        flushUploads(frame);
        retire(frame.uploads);
        waitIdle();

        std::vector<std::byte> pixels(size);
        std::memcpy(pixels.data(), buffer->contents(), size);
        return pixels;
    }

    core::types::Vector2i MetalPlatform::textureSize(TextureHandle handle) const
    {
        return texture(handle).desc.size;
    }

    SamplerHandle MetalPlatform::samplerCreate(const SamplerDesc &desc)
    {
        if (m_free_sampler_indices.empty())
            throw std::length_error("samplerCreate: the bindless sampler table is full");

        Pool pool;
        NS::SharedPtr<MTL::SamplerDescriptor> descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
        const MTL::SamplerAddressMode address = metal::ToMtlAddressMode(desc.address);
        descriptor->setMinFilter(metal::ToMtlFilter(desc.filter));
        descriptor->setMagFilter(metal::ToMtlFilter(desc.filter));
        descriptor->setMipFilter(MTL::SamplerMipFilterNotMipmapped);
        descriptor->setSAddressMode(address);
        descriptor->setTAddressMode(address);
        descriptor->setRAddressMode(address);
        descriptor->setSupportArgumentBuffers(true);

        Sampler sampler{ .sampler = NS::TransferPtr(m_device->newSamplerState(descriptor.get())) };
        if (!sampler.sampler)
            throw std::runtime_error("samplerCreate: failed to create the sampler");
        sampler.bindless = m_free_sampler_indices.back();
        m_free_sampler_indices.pop_back();
        writeBindless(sampler_table_offset + (sampler.bindless * bindless_entry_size), sampler.sampler->gpuResourceID()._impl);
        return m_samplers.insert(std::move(sampler));
    }

    ShaderModuleHandle MetalPlatform::shaderModuleCreate(const ShaderBlob &blob)
    {
        if (blob.target != ShaderTarget::Msl)
            throw std::invalid_argument("shaderModuleCreate: the Metal platform only accepts MSL source");
        if (blob.code.empty())
            throw std::invalid_argument("shaderModuleCreate: MSL source is empty");

        Pool pool;
        const std::string source(reinterpret_cast<const char *>(blob.code.data()), blob.code.size());
        NS::SharedPtr<MTL::CompileOptions> options = NS::TransferPtr(MTL::CompileOptions::alloc()->init());
        options->setLanguageVersion(MTL::LanguageVersion3_0);

        NS::Error *error = nullptr;
        NS::SharedPtr<MTL::Library> library = NS::TransferPtr(m_device->newLibrary(ToNsString(source), options.get(), &error));
        if (!library)
            throw MetalError(std::format("newLibrary ({})", blob.debug_name.empty() ? "shader" : blob.debug_name), error);
        if (error)
            spdlog::debug("Metal compiler output for {}: {}", blob.debug_name, metal::ErrorString(error));
        if (!blob.debug_name.empty())
            library->setLabel(ToNsString(blob.debug_name));
        return m_modules.insert(std::move(library));
    }

    PipelineHandle MetalPlatform::graphicsPipelineCreate(const GraphicsPipelineDesc &desc)
    {
        const NS::SharedPtr<MTL::Library> *vertex_library = m_modules.get(desc.vertex.module);
        const NS::SharedPtr<MTL::Library> *fragment_library = m_modules.get(desc.fragment.module);
        if (!vertex_library || !fragment_library)
            throw std::invalid_argument("graphicsPipelineCreate: unknown shader module");
        const MTL::PixelFormat color_format = metal::ToMtlFormat(desc.color_format);
        if (color_format == MTL::PixelFormatInvalid)
            throw std::invalid_argument("graphicsPipelineCreate: color format is undefined");

        Pool pool;
        NS::SharedPtr<MTL::Function> vertex = NS::TransferPtr((*vertex_library)->newFunction(ToNsString(desc.vertex.entry_point)));
        NS::SharedPtr<MTL::Function> fragment = NS::TransferPtr((*fragment_library)->newFunction(ToNsString(desc.fragment.entry_point)));
        if (!vertex || !fragment)
            throw std::invalid_argument(
                std::format("graphicsPipelineCreate: entry point {} or {} not found", desc.vertex.entry_point, desc.fragment.entry_point));

        NS::SharedPtr<MTL::RenderPipelineDescriptor> descriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
        descriptor->setVertexFunction(vertex.get());
        descriptor->setFragmentFunction(fragment.get());
        descriptor->setInputPrimitiveTopology(metal::ToMtlTopologyClass(desc.topology));
        MTL::RenderPipelineColorAttachmentDescriptor *attachment = descriptor->colorAttachments()->object(0);
        attachment->setPixelFormat(color_format);
        metal::ApplyBlend(attachment, desc.blend);
        if (!desc.debug_name.empty())
            descriptor->setLabel(ToNsString(desc.debug_name));
        if (m_archive)
            descriptor->setBinaryArchives(NS::Array::array(m_archive.get()));

        NS::Error *error = nullptr;
        Pipeline pipeline{ .state = NS::TransferPtr(m_device->newRenderPipelineState(descriptor.get(), &error)),
                           .primitive = metal::ToMtlPrimitive(desc.topology) };
        if (!pipeline.state)
            throw MetalError(std::format("newRenderPipelineState ({})", desc.debug_name), error);

        if (m_archive) {
            error = nullptr;
            if (m_archive->addRenderPipelineFunctions(descriptor.get(), &error))
                m_archive_dirty = true;
            else
                spdlog::debug("Failed to add {} to the Metal pipeline cache: {}", desc.debug_name, metal::ErrorString(error));
        }
        return m_pipelines.insert(std::move(pipeline));
    }

    void MetalPlatform::destroy(BufferHandle buffer)
    {
        std::optional<Buffer> entry = m_buffers.remove(buffer);
        if (!entry)
            throw std::invalid_argument("destroy: unknown buffer");
        defer([this, entry = std::move(*entry)] {
            if (entry.bindless != invalid_index) {
                evict(entry.buffer.get());
                writeBindless(buffer_table_offset + (entry.bindless * bindless_entry_size), 0);
                m_free_buffer_indices.push_back(entry.bindless);
            }
        });
    }

    void MetalPlatform::destroy(TextureHandle handle)
    {
        const Texture *entry = m_textures.get(handle);
        if (!entry || entry->swapchain_image)
            throw std::invalid_argument("destroy: unknown texture");

        Texture removed = std::move(*m_textures.remove(handle));
        m_written.erase(handle.value);
        defer([this, removed = std::move(removed)] {
            if (removed.bindless != invalid_index) {
                evict(removed.texture.get());
                writeBindless(texture_table_offset + (removed.bindless * bindless_entry_size), 0);
                m_free_texture_indices.push_back(removed.bindless);
            }
        });
    }

    void MetalPlatform::destroy(SamplerHandle sampler)
    {
        std::optional<Sampler> entry = m_samplers.remove(sampler);
        if (!entry)
            throw std::invalid_argument("destroy: unknown sampler");
        defer([this, entry = std::move(*entry)] {
            writeBindless(sampler_table_offset + (entry.bindless * bindless_entry_size), 0);
            m_free_sampler_indices.push_back(entry.bindless);
        });
    }

    void MetalPlatform::destroy(ShaderModuleHandle module)
    {
        std::optional<NS::SharedPtr<MTL::Library>> entry = m_modules.remove(module);
        if (!entry)
            throw std::invalid_argument("destroy: unknown shader module");
        defer([entry = std::move(*entry)] {});
    }

    void MetalPlatform::destroy(PipelineHandle pipeline)
    {
        std::optional<Pipeline> entry = m_pipelines.remove(pipeline);
        if (!entry)
            throw std::invalid_argument("destroy: unknown pipeline");
        defer([entry = std::move(*entry)] {});
    }

    std::uint32_t MetalPlatform::bindlessIndex(TextureHandle handle) const
    {
        const Texture &entry = texture(handle);
        if (entry.bindless == invalid_index)
            throw std::logic_error("bindlessIndex: texture was not created with Sampled usage");
        return entry.bindless;
    }

    std::uint32_t MetalPlatform::bindlessIndex(SamplerHandle sampler) const
    {
        return m_samplers.at(sampler).bindless;
    }

    std::uint32_t MetalPlatform::bindlessIndex(BufferHandle buffer) const
    {
        const Buffer &entry = m_buffers.at(buffer);
        if (entry.bindless == invalid_index)
            throw std::logic_error("bindlessIndex: buffer was not created with Storage usage");
        return entry.bindless;
    }

    void MetalPlatform::frameBegin()
    {
        if (m_in_frame)
            throw std::logic_error("frameBegin called twice without frameEnd");

        Frame &frame = prepareFrame(m_frame_number + 1);
        flushUploads(frame);

        m_frame_pool = NS::AutoreleasePool::alloc()->init();
        m_frame_number = frame.number;
        m_in_frame = true;
        m_submitted = false;
        try {
            frame.commands = NS::RetainPtr(newCommandBuffer(std::format("frame {}", m_frame_number)));
        } catch (...) {
            m_frame_pool->release();
            m_frame_pool = nullptr;
            m_in_frame = false;
            throw;
        }
        m_command_list->begin(frame.commands.get());
    }

    CommandList &MetalPlatform::commandListAcquire(QueueType queue)
    {
        if (!m_in_frame || m_submitted)
            throw std::logic_error("commandListAcquire called outside a frame or after submit");
        return *m_command_list;
    }

    void MetalPlatform::submit(std::span<CommandList *const> lists)
    {
        if (!m_in_frame || m_submitted)
            throw std::logic_error("submit called outside a frame or twice in one frame");
        for (CommandList *list : lists)
            if (list != m_command_list.get())
                throw std::invalid_argument("submit: command list does not belong to this frame");
        submitFrame(lists);
    }

    void MetalPlatform::submitFrame(std::span<CommandList *const> lists)
    {
        if (m_command_list->inRenderPass()) {
            if (!lists.empty())
                throw std::logic_error("submit: a render pass is still open");
            spdlog::error("The frame ended with an open render pass, closing it");
            m_command_list->renderPassEnd();
        }

        Frame &frame = m_frames[m_frame_number % frames_in_flight];
        m_command_list->end();
        m_swapchains.forEach([&frame](SwapchainHandle, Swapchain &swapchain) {
            if (swapchain.acquired && swapchain.drawable)
                frame.commands->presentDrawable(swapchain.drawable.get());
        });

        m_submitted = true;
        commitResidency();
        frame.commands->commit();
    }

    void MetalPlatform::frameEnd()
    {
        if (!m_in_frame)
            throw std::logic_error("frameEnd called without frameBegin");

        auto finish = [this] {
            m_swapchains.forEach([this](SwapchainHandle, Swapchain &swapchain) {
                if (!swapchain.acquired)
                    return;
                swapchain.acquired = false;
                swapchain.presented = false;
                swapchain.drawable.reset();
                if (Texture *image = m_textures.get(swapchain.image))
                    image->texture.reset();
            });
            if (m_frame_pool) {
                m_frame_pool->release();
                m_frame_pool = nullptr;
            }
            m_in_frame = false;
        };

        try {
            if (!m_submitted)
                submitFrame({});
        } catch (...) {
            finish();
            throw;
        }
        finish();
    }

    bool MetalPlatform::validationEnabled() const noexcept
    {
        return m_validation;
    }

    std::uint32_t MetalPlatform::validationErrorCount() const noexcept
    {
        return m_gpu_errors;
    }

    bool MetalPlatform::usesResidencySets() const noexcept
    {
        return static_cast<bool>(m_residency);
    }

    const std::filesystem::path &MetalPlatform::pipelineArchivePath() const noexcept
    {
        return m_archive_path;
    }
} // namespace atmo::server::rendering
