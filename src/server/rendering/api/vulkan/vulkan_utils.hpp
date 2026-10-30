#pragma once

#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

#include "server/rendering/platform/rendering_platform.hpp"
#include "vulkan_includes.hpp"

namespace atmo::server::rendering::vulkan
{
    class VulkanError : public std::runtime_error
    {
    public:
        VulkanError(VkResult result, std::string_view call);

        [[nodiscard]] VkResult result() const noexcept;

    private:
        VkResult m_result;
    };

    [[nodiscard]] std::string_view ResultName(VkResult result) noexcept;

    void Check(VkResult result, std::string_view call);

    struct StateInfo {
        VkImageLayout layout;
        VkPipelineStageFlags2 stages;
        VkAccessFlags2 access;
    };

    [[nodiscard]] StateInfo SourceState(ResourceState state) noexcept;
    [[nodiscard]] StateInfo DestinationState(ResourceState state) noexcept;

    [[nodiscard]] VkFormat ToVkFormat(TextureFormat format) noexcept;
    [[nodiscard]] TextureFormat FromVkFormat(VkFormat format) noexcept;
    [[nodiscard]] VkImageUsageFlags ToVkImageUsage(TextureUsage usage) noexcept;
    [[nodiscard]] VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage) noexcept;
    [[nodiscard]] VkFilter ToVkFilter(Filter filter) noexcept;
    [[nodiscard]] VkSamplerAddressMode ToVkAddressMode(AddressMode mode) noexcept;
    [[nodiscard]] VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology) noexcept;
    [[nodiscard]] VkPipelineColorBlendAttachmentState ToVkBlend(BlendMode mode) noexcept;
    [[nodiscard]] VkAttachmentLoadOp ToVkLoadOp(LoadOp op) noexcept;
    [[nodiscard]] VkAttachmentStoreOp ToVkStoreOp(StoreOp op) noexcept;
} // namespace atmo::server::rendering::vulkan
