#include <array>
#include <span>

#include "rendering_platform.hpp"
#include "server/rendering/api/null/null_platform.hpp"
#include "spdlog/spdlog.h"

#if defined(ATMO_RENDERING_VULKAN)
#include "server/rendering/api/vulkan/vulkan_platform.hpp"
#endif

namespace atmo::server::rendering
{
    std::string_view PlatformKindName(PlatformKind kind) noexcept
    {
        switch (kind) {
            case PlatformKind::Null:
                return "null";
            case PlatformKind::Vulkan:
                return "vulkan";
            case PlatformKind::Metal:
                return "metal";
        }
        return "unknown";
    }

    std::optional<PlatformKind> ParsePlatformKind(std::string_view name) noexcept
    {
        for (PlatformKind kind : { PlatformKind::Null, PlatformKind::Vulkan, PlatformKind::Metal })
            if (name == PlatformKindName(kind))
                return kind;
        return std::nullopt;
    }

    std::size_t BytesPerPixel(TextureFormat format) noexcept
    {
        switch (format) {
            case TextureFormat::Undefined:
                return 0;
            case TextureFormat::R8Unorm:
                return 1;
            case TextureFormat::Rgba8Unorm:
            case TextureFormat::Rgba8Srgb:
            case TextureFormat::Bgra8Unorm:
            case TextureFormat::Bgra8Srgb:
                return 4;
        }
        return 0;
    }

    static void ReportUnbuilt(PlatformKind kind, bool requested)
    {
        if (requested)
            spdlog::warn("This build does not include the {} rendering platform", PlatformKindName(kind));
        else
            spdlog::debug("This build does not include the {} rendering platform", PlatformKindName(kind));
    }

    static std::unique_ptr<RenderingPlatform> Instantiate(PlatformKind kind, bool requested)
    {
        switch (kind) {
            case PlatformKind::Null:
                return std::make_unique<NullPlatform>();
            case PlatformKind::Vulkan:
#if defined(ATMO_RENDERING_VULKAN)
                return std::make_unique<VulkanPlatform>();
#else
                ReportUnbuilt(kind, requested);
                return nullptr;
#endif
            case PlatformKind::Metal:
                ReportUnbuilt(kind, requested);
                return nullptr;
        }
        return nullptr;
    }

    static std::unique_ptr<RenderingPlatform> TryCreate(PlatformKind kind, const RenderingSettings &settings, bool requested)
    {
        auto platform = Instantiate(kind, requested);
        if (!platform)
            return nullptr;

        try {
            if (platform->initialize(settings))
                return platform;
        } catch (const std::exception &e) {
            spdlog::error("Rendering platform {} failed to initialize: {}", platform->name(), e.what());
        }

        platform->shutdown();
        return nullptr;
    }

    std::span<const PlatformKind> AutomaticPlatformOrder() noexcept
    {
#if defined(__APPLE__)
        static constexpr std::array order{ PlatformKind::Metal, PlatformKind::Null };
#else
        static constexpr std::array order{ PlatformKind::Null };
#endif
        return order;
    }

    std::unique_ptr<RenderingPlatform> CreatePlatform(const RenderingSettings &settings)
    {
        if (settings.platform) {
            if (auto platform = TryCreate(*settings.platform, settings, true))
                return platform;
            spdlog::warn("Rendering platform {} is unavailable, falling back to automatic selection", PlatformKindName(*settings.platform));
        }

        for (PlatformKind kind : AutomaticPlatformOrder()) {
            if (settings.platform == kind)
                continue;
            if (auto platform = TryCreate(kind, settings, false))
                return platform;
            spdlog::debug("Rendering platform {} is unavailable", PlatformKindName(kind));
        }

        return nullptr;
    }
} // namespace atmo::server::rendering
