#include <algorithm>
#include <stdexcept>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_vulkan.h"
#include "spdlog/spdlog.h"
#include "vulkan_platform.hpp"
#include "vulkan_utils.hpp"

#if defined(__APPLE__)
#include <vulkan/vulkan_metal.h>
#endif

namespace atmo::server::rendering
{
    using vulkan::Check;

    namespace
    {
        VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR> &formats)
        {
            for (VkFormat preferred : { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM }) {
                auto it = std::ranges::find_if(formats, [preferred](const VkSurfaceFormatKHR &format) {
                    return format.format == preferred && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                });
                if (it != formats.end())
                    return *it;
            }
            return formats.front();
        }

        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR> &modes, bool vsync)
        {
            if (vsync)
                return VK_PRESENT_MODE_FIFO_KHR;
            for (VkPresentModeKHR preferred : { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR })
                if (std::ranges::find(modes, preferred) != modes.end())
                    return preferred;
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkCompositeAlphaFlagBitsKHR ChooseCompositeAlpha(VkCompositeAlphaFlagsKHR supported)
        {
            for (VkCompositeAlphaFlagBitsKHR preferred : { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR })
                if ((supported & preferred) != 0)
                    return preferred;
            return static_cast<VkCompositeAlphaFlagBitsKHR>(supported & (~supported + 1u));
        }
    } // namespace

    SwapchainHandle VulkanPlatform::swapchainCreate(const NativeWindow &window, const SwapchainDesc &desc)
    {
        if (!m_caps.presentation) {
            spdlog::error("swapchainCreate: this Vulkan instance cannot present to windows");
            return {};
        }

        Swapchain swapchain{ .window = window.sdl_window, .requested_size = desc.size, .vsync = desc.vsync };
        if (!surfaceCreate(window, swapchain))
            return {};

        VkBool32 supported = VK_FALSE;
        Check(vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, m_queue_family, swapchain.surface, &supported), "vkGetPhysicalDeviceSurfaceSupportKHR");
        if (!supported) {
            surfaceDestroy(swapchain);
            spdlog::error("The selected GPU queue cannot present to this window");
            return {};
        }

        const VkSemaphoreCreateInfo semaphore_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        for (VkSemaphore &semaphore : swapchain.acquire_semaphores)
            Check(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &semaphore), "vkCreateSemaphore");

        swapchain.needs_recreate = true;
        SwapchainHandle handle = m_swapchains.insert(std::move(swapchain));
        try {
            swapchainBuild(m_swapchains.at(handle));
        } catch (...) {
            swapchainDestroy(handle);
            throw;
        }
        return handle;
    }

    bool VulkanPlatform::surfaceCreate(const NativeWindow &window, Swapchain &swapchain)
    {
#if defined(__APPLE__)
        if (window.metal_layer) {
            auto create = reinterpret_cast<PFN_vkCreateMetalSurfaceEXT>(vkGetInstanceProcAddr(m_instance, "vkCreateMetalSurfaceEXT"));
            if (!create) {
                spdlog::error("VK_EXT_metal_surface is not enabled on this Vulkan instance");
                return false;
            }
            const VkMetalSurfaceCreateInfoEXT info{
                .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
                .pLayer = static_cast<const CAMetalLayer *>(window.metal_layer),
            };
            Check(create(m_instance, &info, nullptr, &swapchain.surface), "vkCreateMetalSurfaceEXT");
            swapchain.sdl_surface = false;
            return true;
        }
#endif
        if (!SDL_Vulkan_CreateSurface(window.sdl_window, m_instance, nullptr, &swapchain.surface)) {
            spdlog::error("Failed to create a Vulkan surface: {}", SDL_GetError());
            return false;
        }
        swapchain.sdl_surface = true;
        return true;
    }

    void VulkanPlatform::surfaceDestroy(const Swapchain &swapchain) noexcept
    {
        if (!swapchain.surface)
            return;
        if (swapchain.sdl_surface)
            SDL_Vulkan_DestroySurface(m_instance, swapchain.surface, nullptr);
        else
            vkDestroySurfaceKHR(m_instance, swapchain.surface, nullptr);
    }

    void VulkanPlatform::swapchainBuild(Swapchain &swapchain)
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, swapchain.surface, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

        VkExtent2D extent = capabilities.currentExtent;
        if (extent.width == UINT32_MAX) {
            extent.width = std::clamp(
                static_cast<std::uint32_t>(std::max(swapchain.requested_size.x, 0)), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(
                static_cast<std::uint32_t>(std::max(swapchain.requested_size.y, 0)), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        if (extent.width == 0 || extent.height == 0) {
            swapchain.needs_recreate = true;
            return;
        }

        std::uint32_t count = 0;
        Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, swapchain.surface, &count, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
        std::vector<VkSurfaceFormatKHR> formats(count);
        Check(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, swapchain.surface, &count, formats.data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
        if (formats.empty())
            throw std::runtime_error("the surface reports no formats");

        Check(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, swapchain.surface, &count, nullptr), "vkGetPhysicalDeviceSurfacePresentModesKHR");
        std::vector<VkPresentModeKHR> modes(count);
        Check(
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, swapchain.surface, &count, modes.data()), "vkGetPhysicalDeviceSurfacePresentModesKHR");

        const VkSurfaceFormatKHR format = ChooseFormat(formats);
        std::uint32_t image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
            image_count = std::min(image_count, capabilities.maxImageCount);

        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if ((capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0)
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        const VkSwapchainKHR old_swapchain = swapchain.swapchain;
        const VkSwapchainCreateInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = swapchain.surface,
            .minImageCount = image_count,
            .imageFormat = format.format,
            .imageColorSpace = format.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = usage,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = ChooseCompositeAlpha(capabilities.supportedCompositeAlpha),
            .presentMode = ChoosePresentMode(modes, swapchain.vsync),
            .clipped = VK_TRUE,
            .oldSwapchain = old_swapchain,
        };
        VkSwapchainKHR created = VK_NULL_HANDLE;
        Check(vkCreateSwapchainKHR(m_device, &info, nullptr, &created), "vkCreateSwapchainKHR");

        swapchainReleaseImages(swapchain);
        if (old_swapchain)
            vkDestroySwapchainKHR(m_device, old_swapchain, nullptr);

        swapchain.swapchain = created;
        swapchain.format = format.format;
        swapchain.extent = extent;

        Check(vkGetSwapchainImagesKHR(m_device, swapchain.swapchain, &count, nullptr), "vkGetSwapchainImagesKHR");
        std::vector<VkImage> images(count);
        Check(vkGetSwapchainImagesKHR(m_device, swapchain.swapchain, &count, images.data()), "vkGetSwapchainImagesKHR");

        const VkSemaphoreCreateInfo semaphore_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        TextureUsage texture_usage = TextureUsage::RenderTarget;
        if ((usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0)
            texture_usage = texture_usage | TextureUsage::CopySrc;

        for (VkImage image : images) {
            Texture texture{
                .image = image,
                .format = format.format,
                .desc = { .size = { static_cast<int>(extent.width), static_cast<int>(extent.height) },
                          .format = vulkan::FromVkFormat(format.format),
                          .usage = texture_usage },
                .swapchain_image = true,
            };
            const VkImageViewCreateInfo view_info{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format.format,
                .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
            };
            Check(vkCreateImageView(m_device, &view_info, nullptr, &texture.view), "vkCreateImageView");
            swapchain.images.push_back(m_textures.insert(texture));

            VkSemaphore semaphore = VK_NULL_HANDLE;
            Check(vkCreateSemaphore(m_device, &semaphore_info, nullptr, &semaphore), "vkCreateSemaphore");
            swapchain.present_semaphores.push_back(semaphore);
        }

        swapchain.needs_recreate = false;
    }

    void VulkanPlatform::swapchainReleaseImages(Swapchain &swapchain) noexcept
    {
        for (TextureHandle image : swapchain.images) {
            if (std::optional<Texture> texture = m_textures.remove(image))
                vkDestroyImageView(m_device, texture->view, nullptr);
        }
        swapchain.images.clear();

        for (VkSemaphore semaphore : swapchain.present_semaphores) vkDestroySemaphore(m_device, semaphore, nullptr);
        swapchain.present_semaphores.clear();
    }

    void VulkanPlatform::swapchainResize(SwapchainHandle handle, core::types::Vector2i size)
    {
        Swapchain &swapchain = m_swapchains.at(handle);
        swapchain.requested_size = size;
        swapchain.needs_recreate = true;
    }

    void VulkanPlatform::swapchainDestroy(SwapchainHandle handle)
    {
        Swapchain *swapchain = m_swapchains.get(handle);
        if (!swapchain)
            throw std::invalid_argument("swapchainDestroy: unknown swapchain");
        if (swapchain->acquired && !swapchain->presented)
            throw std::logic_error("swapchainDestroy: swapchain is acquired in the current frame");

        waitIdle();
        swapchainReleaseImages(*swapchain);
        for (VkSemaphore semaphore : swapchain->acquire_semaphores)
            if (semaphore)
                vkDestroySemaphore(m_device, semaphore, nullptr);
        if (swapchain->swapchain)
            vkDestroySwapchainKHR(m_device, swapchain->swapchain, nullptr);
        surfaceDestroy(*swapchain);
        m_swapchains.remove(handle);
    }

    TextureFormat VulkanPlatform::swapchainFormat(SwapchainHandle handle) const
    {
        return vulkan::FromVkFormat(m_swapchains.at(handle).format);
    }

    TextureHandle VulkanPlatform::swapchainAcquire(SwapchainHandle handle)
    {
        if (!m_in_frame || m_submitted)
            throw std::logic_error("swapchainAcquire called outside a frame or after submit");

        Swapchain &swapchain = m_swapchains.at(handle);
        if (swapchain.acquired)
            throw std::logic_error("swapchainAcquire: swapchain already acquired this frame");

        for (int attempt = 0; attempt < 2; ++attempt) {
            if (swapchain.needs_recreate || !swapchain.swapchain) {
                VkSurfaceCapabilitiesKHR capabilities{};
                Check(
                    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, swapchain.surface, &capabilities),
                    "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
                if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0)
                    return {};

                waitIdle();
                swapchainBuild(swapchain);
                if (!swapchain.swapchain || swapchain.needs_recreate)
                    return {};
            }

            VkSemaphore semaphore = swapchain.acquire_semaphores[m_frame_number % frames_in_flight];
            const VkResult result = vkAcquireNextImageKHR(m_device, swapchain.swapchain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &swapchain.image_index);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                swapchain.needs_recreate = true;
                continue;
            }
            if (result == VK_SUBOPTIMAL_KHR)
                swapchain.needs_recreate = true;
            else
                Check(result, "vkAcquireNextImageKHR");

            swapchain.acquired = true;
            swapchain.waited = false;
            swapchain.presented = false;

            TextureHandle image = swapchain.images[swapchain.image_index];
            texture(image).state = ResourceState::Undefined;
            return image;
        }

        return {};
    }

    void VulkanPlatform::swapchainPresent(SwapchainHandle handle)
    {
        if (!m_in_frame || !m_submitted)
            throw std::logic_error("swapchainPresent must follow submit in the same frame");

        Swapchain &swapchain = m_swapchains.at(handle);
        if (!swapchain.acquired || !swapchain.waited)
            throw std::logic_error("swapchainPresent: swapchain was not acquired before submit");

        const VkPresentInfoKHR info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &swapchain.present_semaphores[swapchain.image_index],
            .swapchainCount = 1,
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &swapchain.image_index,
        };

        swapchain.acquired = false;
        swapchain.presented = true;

        const VkResult result = vkQueuePresentKHR(m_queue, &info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            swapchain.needs_recreate = true;
        else
            Check(result, "vkQueuePresentKHR");
    }
} // namespace atmo::server::rendering
