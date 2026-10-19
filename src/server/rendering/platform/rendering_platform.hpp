#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "SDL3/SDL_video.h"
#include "core/types.hpp"
#include "server/rendering/types.hpp"

namespace atmo::server::rendering
{
    template <typename Tag> struct GpuHandle {
        std::uint32_t value = 0;

        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            return value != 0;
        }

        constexpr bool operator==(const GpuHandle &) const noexcept = default;
    };

    using BufferHandle = GpuHandle<struct BufferTag>;
    using TextureHandle = GpuHandle<struct TextureTag>;
    using SamplerHandle = GpuHandle<struct SamplerTag>;
    using ShaderModuleHandle = GpuHandle<struct ShaderModuleTag>;
    using PipelineHandle = GpuHandle<struct PipelineTag>;
    using SwapchainHandle = GpuHandle<struct SwapchainTag>;

    enum class ShaderStage : std::uint8_t {
        Vertex,
        Fragment,
        Compute,
    };

    enum class QueueType : std::uint8_t {
        Graphics,
    };

    enum class TextureFormat : std::uint8_t {
        Undefined,
        R8Unorm,
        Rgba8Unorm,
        Rgba8Srgb,
        Bgra8Unorm,
        Bgra8Srgb,
    };

    enum class TextureUsage : std::uint8_t {
        None = 0,
        Sampled = 1 << 0,
        RenderTarget = 1 << 1,
        CopySrc = 1 << 2,
        CopyDst = 1 << 3,
    };

    enum class BufferUsage : std::uint8_t {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        CopySrc = 1 << 4,
        CopyDst = 1 << 5,
    };

    template <typename E>
    concept FlagEnum = std::is_same_v<E, TextureUsage> || std::is_same_v<E, BufferUsage>;

    template <FlagEnum E> [[nodiscard]] constexpr E operator|(E a, E b) noexcept
    {
        return static_cast<E>(std::to_underlying(a) | std::to_underlying(b));
    }

    template <FlagEnum E> [[nodiscard]] constexpr bool HasFlag(E value, E flag) noexcept
    {
        return (std::to_underlying(value) & std::to_underlying(flag)) == std::to_underlying(flag);
    }

    enum class MemoryType : std::uint8_t {
        GpuOnly,
        Upload,
        Readback,
    };

    enum class ResourceState : std::uint8_t {
        Undefined,
        RenderTarget,
        ShaderRead,
        CopySrc,
        CopyDst,
        Present,
    };

    enum class LoadOp : std::uint8_t {
        Load,
        Clear,
        DontCare,
    };

    enum class StoreOp : std::uint8_t {
        Store,
        DontCare,
    };

    enum class Filter : std::uint8_t {
        Nearest,
        Linear,
    };

    enum class AddressMode : std::uint8_t {
        ClampToEdge,
        Repeat,
        MirroredRepeat,
    };

    enum class BlendMode : std::uint8_t {
        Opaque,
        Alpha,
        PremultipliedAlpha,
        Additive,
    };

    enum class PrimitiveTopology : std::uint8_t {
        TriangleList,
        TriangleStrip,
        LineList,
        LineStrip,
    };

    enum class IndexType : std::uint8_t {
        Uint16,
        Uint32,
    };

    struct NativeWindow {
        SDL_Window *sdl_window = nullptr;
        void *metal_layer = nullptr;
    };

    struct PlatformCaps {
        std::string device_name;
        std::uint32_t max_texture_size = 0;
        std::uint32_t max_bindless_textures = 0;
        std::uint32_t max_bindless_samplers = 0;
        std::uint32_t max_bindless_buffers = 0;
        std::uint32_t max_push_constant_size = 0;
        std::uint32_t frames_in_flight = 1;
        bool presentation = false;
    };

    struct Region2i {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int32_t width = 0;
        std::int32_t height = 0;
    };

    struct SwapchainDesc {
        core::types::Vector2i size{};
        bool vsync = true;
    };

    struct BufferDesc {
        std::uint64_t size = 0;
        BufferUsage usage = BufferUsage::None;
        MemoryType memory = MemoryType::GpuOnly;
        std::string_view debug_name;
    };

    struct TextureDesc {
        core::types::Vector2i size{};
        TextureFormat format = TextureFormat::Rgba8Unorm;
        TextureUsage usage = TextureUsage::Sampled | TextureUsage::CopyDst;
        std::string_view debug_name;
    };

    struct SamplerDesc {
        Filter filter = Filter::Linear;
        AddressMode address = AddressMode::ClampToEdge;
    };

    struct ShaderBlob {
        ShaderTarget target = ShaderTarget::None;
        std::span<const std::byte> code;
        std::string_view debug_name;
    };

    struct ShaderStageDesc {
        ShaderModuleHandle module;
        std::string_view entry_point;
    };

    struct GraphicsPipelineDesc {
        ShaderStageDesc vertex;
        ShaderStageDesc fragment;
        TextureFormat color_format = TextureFormat::Undefined;
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        BlendMode blend = BlendMode::Alpha;
        std::string_view debug_name;
    };

    struct ColorAttachment {
        TextureHandle texture;
        LoadOp load = LoadOp::Clear;
        StoreOp store = StoreOp::Store;
        std::array<float, 4> clear_color{ 0.0f, 0.0f, 0.0f, 1.0f };
    };

    struct RenderPassDesc {
        std::span<const ColorAttachment> color_attachments;
        std::string_view debug_name;
    };

    struct Viewport {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float min_depth = 0.0f;
        float max_depth = 1.0f;
    };

    struct TextureBarrier {
        TextureHandle texture;
        ResourceState after = ResourceState::Undefined;
    };

    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void barrier(std::span<const TextureBarrier> textures) = 0;
        virtual void renderPassBegin(const RenderPassDesc &desc) = 0;
        virtual void renderPassEnd() = 0;
        virtual void pipelineBind(PipelineHandle pipeline) = 0;
        virtual void constantsPush(std::span<const std::byte> data) = 0;
        virtual void indexBufferBind(BufferHandle buffer, IndexType type, std::uint64_t offset) = 0;
        virtual void viewportSet(const Viewport &viewport) = 0;
        virtual void scissorSet(const Region2i &scissor) = 0;
        virtual void draw(std::uint32_t vertices, std::uint32_t instances, std::uint32_t first_vertex, std::uint32_t first_instance) = 0;
        virtual void
        drawIndexed(std::uint32_t indices, std::uint32_t instances, std::uint32_t first_index, std::int32_t vertex_offset, std::uint32_t first_instance) = 0;
        virtual void debugLabelPush(std::string_view label) = 0;
        virtual void debugLabelPop() = 0;
    };

    class RenderingPlatform
    {
    public:
        virtual ~RenderingPlatform() = default;

        [[nodiscard]] virtual PlatformKind kind() const noexcept = 0;
        [[nodiscard]] virtual std::string_view name() const noexcept = 0;

        virtual bool initialize(const RenderingSettings &settings) = 0;
        virtual void shutdown() = 0;

        [[nodiscard]] virtual SDL_WindowFlags windowFlags() const noexcept = 0;
        [[nodiscard]] virtual const PlatformCaps &caps() const noexcept = 0;
        [[nodiscard]] virtual ShaderTarget shaderTarget() const noexcept = 0;

        virtual SwapchainHandle swapchainCreate(const NativeWindow &window, const SwapchainDesc &desc) = 0;
        virtual void swapchainResize(SwapchainHandle swapchain, core::types::Vector2i size) = 0;
        virtual void swapchainDestroy(SwapchainHandle swapchain) = 0;
        [[nodiscard]] virtual TextureFormat swapchainFormat(SwapchainHandle swapchain) const = 0;
        virtual TextureHandle swapchainAcquire(SwapchainHandle swapchain) = 0;
        virtual void swapchainPresent(SwapchainHandle swapchain) = 0;

        virtual BufferHandle bufferCreate(const BufferDesc &desc) = 0;
        virtual void bufferWrite(BufferHandle buffer, std::uint64_t offset, std::span<const std::byte> data) = 0;
        virtual TextureHandle textureCreate(const TextureDesc &desc) = 0;
        virtual void textureUpload(TextureHandle texture, const Region2i &region, std::span<const std::byte> pixels) = 0;
        virtual std::vector<std::byte> textureReadback(TextureHandle texture) = 0;
        [[nodiscard]] virtual core::types::Vector2i textureSize(TextureHandle texture) const = 0;
        virtual SamplerHandle samplerCreate(const SamplerDesc &desc) = 0;
        virtual ShaderModuleHandle shaderModuleCreate(const ShaderBlob &blob) = 0;
        virtual PipelineHandle graphicsPipelineCreate(const GraphicsPipelineDesc &desc) = 0;

        virtual void destroy(BufferHandle buffer) = 0;
        virtual void destroy(TextureHandle texture) = 0;
        virtual void destroy(SamplerHandle sampler) = 0;
        virtual void destroy(ShaderModuleHandle module) = 0;
        virtual void destroy(PipelineHandle pipeline) = 0;

        [[nodiscard]] virtual std::uint32_t bindlessIndex(TextureHandle texture) const = 0;
        [[nodiscard]] virtual std::uint32_t bindlessIndex(SamplerHandle sampler) const = 0;
        [[nodiscard]] virtual std::uint32_t bindlessIndex(BufferHandle buffer) const = 0;

        virtual void frameBegin() = 0;
        virtual CommandList &commandListAcquire(QueueType queue) = 0;
        virtual void submit(std::span<CommandList *const> lists) = 0;
        virtual void frameEnd() = 0;
    };

    [[nodiscard]] std::size_t BytesPerPixel(TextureFormat format) noexcept;

    [[nodiscard]] std::span<const PlatformKind> AutomaticPlatformOrder() noexcept;

    std::unique_ptr<RenderingPlatform> CreatePlatform(const RenderingSettings &settings);
} // namespace atmo::server::rendering
