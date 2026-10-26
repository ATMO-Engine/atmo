#pragma once

#include "server/rendering/types.hpp"

#if defined(ATMO_RENDERING_VULKAN)
#include "server/rendering/api/vulkan/vulkan_platform.hpp"
#endif
#if defined(ATMO_RENDERING_METAL)
#include "server/rendering/api/metal/metal_platform.hpp"
#endif

template <typename Platform> struct GpuPlatformTraits;

#if defined(ATMO_RENDERING_VULKAN)
using atmo::server::rendering::VulkanPlatform;

template <> struct GpuPlatformTraits<VulkanPlatform> {
    static constexpr atmo::server::rendering::PlatformKind kind = atmo::server::rendering::PlatformKind::Vulkan;
    static constexpr const char *unavailable = "No Vulkan 1.3 device with the required features";
};
#endif

#if defined(ATMO_RENDERING_METAL)
using atmo::server::rendering::MetalPlatform;

template <> struct GpuPlatformTraits<MetalPlatform> {
    static constexpr atmo::server::rendering::PlatformKind kind = atmo::server::rendering::PlatformKind::Metal;
    static constexpr const char *unavailable = "No Metal 3 device with tier 2 argument buffers";
};
#endif

#if defined(ATMO_RENDERING_VULKAN) && defined(ATMO_RENDERING_METAL)
#define ATMO_GPU_PLATFORMS VulkanPlatform, MetalPlatform
#elif defined(ATMO_RENDERING_VULKAN)
#define ATMO_GPU_PLATFORMS VulkanPlatform
#elif defined(ATMO_RENDERING_METAL)
#define ATMO_GPU_PLATFORMS MetalPlatform
#endif
