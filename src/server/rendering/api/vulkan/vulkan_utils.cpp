#include "vulkan_utils.hpp"

#include <format>

namespace atmo::server::rendering::vulkan
{
    VulkanError::VulkanError(VkResult result, std::string_view call) :
        std::runtime_error(std::format("{} failed: {}", call, ResultName(result))), m_result(result)
    {
    }

    VkResult VulkanError::result() const noexcept
    {
        return m_result;
    }

    std::string_view ResultName(VkResult result) noexcept
    {
        switch (result) {
            case VK_SUCCESS:
                return "VK_SUCCESS";
            case VK_NOT_READY:
                return "VK_NOT_READY";
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
            case VK_SUBOPTIMAL_KHR:
                return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return "VK_ERROR_OUT_OF_POOL_MEMORY";
            case VK_ERROR_SURFACE_LOST_KHR:
                return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_ERROR_OUT_OF_DATE_KHR:
                return "VK_ERROR_OUT_OF_DATE_KHR";
            default:
                return "unknown VkResult";
        }
    }

    void Check(VkResult result, std::string_view call)
    {
        if (result < VK_SUCCESS)
            throw VulkanError(result, call);
    }

    StateInfo SourceState(ResourceState state) noexcept
    {
        switch (state) {
            case ResourceState::Undefined:
                return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE };
            case ResourceState::RenderTarget:
                return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT };
            case ResourceState::ShaderRead:
                return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                         VK_ACCESS_2_NONE };
            case ResourceState::CopySrc:
                return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_NONE };
            case ResourceState::CopyDst:
                return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
            case ResourceState::Present:
                return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE };
        }
        return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE };
    }

    StateInfo DestinationState(ResourceState state) noexcept
    {
        switch (state) {
            case ResourceState::Undefined:
                return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE };
            case ResourceState::RenderTarget:
                return { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT };
            case ResourceState::ShaderRead:
                return { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                         VK_ACCESS_2_SHADER_SAMPLED_READ_BIT };
            case ResourceState::CopySrc:
                return { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT };
            case ResourceState::CopyDst:
                return { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
            case ResourceState::Present:
                return { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE };
        }
        return { VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE };
    }

    VkFormat ToVkFormat(TextureFormat format) noexcept
    {
        switch (format) {
            case TextureFormat::Undefined:
                return VK_FORMAT_UNDEFINED;
            case TextureFormat::R8Unorm:
                return VK_FORMAT_R8_UNORM;
            case TextureFormat::Rgba8Unorm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::Rgba8Srgb:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::Bgra8Unorm:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::Bgra8Srgb:
                return VK_FORMAT_B8G8R8A8_SRGB;
        }
        return VK_FORMAT_UNDEFINED;
    }

    TextureFormat FromVkFormat(VkFormat format) noexcept
    {
        switch (format) {
            case VK_FORMAT_R8_UNORM:
                return TextureFormat::R8Unorm;
            case VK_FORMAT_R8G8B8A8_UNORM:
                return TextureFormat::Rgba8Unorm;
            case VK_FORMAT_R8G8B8A8_SRGB:
                return TextureFormat::Rgba8Srgb;
            case VK_FORMAT_B8G8R8A8_UNORM:
                return TextureFormat::Bgra8Unorm;
            case VK_FORMAT_B8G8R8A8_SRGB:
                return TextureFormat::Bgra8Srgb;
            default:
                return TextureFormat::Undefined;
        }
    }

    VkImageUsageFlags ToVkImageUsage(TextureUsage usage) noexcept
    {
        VkImageUsageFlags flags = 0;
        if (HasFlag(usage, TextureUsage::Sampled))
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (HasFlag(usage, TextureUsage::RenderTarget))
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (HasFlag(usage, TextureUsage::CopySrc))
            flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (HasFlag(usage, TextureUsage::CopyDst))
            flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        return flags;
    }

    VkBufferUsageFlags ToVkBufferUsage(BufferUsage usage) noexcept
    {
        VkBufferUsageFlags flags = 0;
        if (HasFlag(usage, BufferUsage::Vertex))
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (HasFlag(usage, BufferUsage::Index))
            flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (HasFlag(usage, BufferUsage::Uniform))
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if (HasFlag(usage, BufferUsage::Storage))
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (HasFlag(usage, BufferUsage::CopySrc))
            flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        if (HasFlag(usage, BufferUsage::CopyDst))
            flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        return flags;
    }

    VkFilter ToVkFilter(Filter filter) noexcept
    {
        return filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    }

    VkSamplerAddressMode ToVkAddressMode(AddressMode mode) noexcept
    {
        switch (mode) {
            case AddressMode::ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case AddressMode::Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case AddressMode::MirroredRepeat:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology) noexcept
    {
        switch (topology) {
            case PrimitiveTopology::TriangleList:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::LineList:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        }
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    VkPipelineColorBlendAttachmentState ToVkBlend(BlendMode mode) noexcept
    {
        VkPipelineColorBlendAttachmentState state{
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        switch (mode) {
            case BlendMode::Opaque:
                state.blendEnable = VK_FALSE;
                break;
            case BlendMode::Alpha:
                break;
            case BlendMode::PremultipliedAlpha:
                state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                break;
            case BlendMode::Additive:
                state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                break;
        }
        return state;
    }

    VkAttachmentLoadOp ToVkLoadOp(LoadOp op) noexcept
    {
        switch (op) {
            case LoadOp::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case LoadOp::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case LoadOp::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    VkAttachmentStoreOp ToVkStoreOp(StoreOp op) noexcept
    {
        return op == StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
} // namespace atmo::server::rendering::vulkan
