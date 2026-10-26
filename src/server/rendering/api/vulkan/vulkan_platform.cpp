#include "vulkan_platform.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "SDL3/SDL_error.h"
#include "SDL3/SDL_filesystem.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_vulkan.h"
#include "spdlog/spdlog.h"
#include "vulkan_utils.hpp"

#if defined(ATMO_PROFILER_TRACY)
#include <tracy/TracyVulkan.hpp>
#endif

namespace atmo::server::rendering
{
    using vulkan::Check;

    namespace
    {
        constexpr std::uint32_t max_bindless_textures = 16384;
        constexpr std::uint32_t max_bindless_samplers = 64;
        constexpr std::uint32_t max_bindless_buffers = 4096;
        constexpr const char *validation_layer = "VK_LAYER_KHRONOS_validation";
        constexpr const char *pipeline_cache_file = "vulkan-pipelines.bin";
        constexpr const char *portability_subset_extension = "VK_KHR_portability_subset";

        bool Contains(const std::vector<VkExtensionProperties> &extensions, std::string_view name)
        {
            return std::ranges::any_of(extensions, [name](const VkExtensionProperties &extension) { return name == extension.extensionName; });
        }

        bool Contains(const std::vector<VkLayerProperties> &layers, std::string_view name)
        {
            return std::ranges::any_of(layers, [name](const VkLayerProperties &layer) { return name == layer.layerName; });
        }

        void AddUnique(std::vector<const char *> &names, const char *name)
        {
            if (std::ranges::none_of(names, [name](const char *existing) { return std::strcmp(existing, name) == 0; }))
                names.push_back(name);
        }

        std::string VersionString(std::uint32_t version)
        {
            return std::format("{}.{}.{}", VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));
        }

        VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment)
        {
            return (value + alignment - 1) & ~(alignment - 1);
        }
    } // namespace

    VulkanPlatform::VulkanPlatform() = default;

    VulkanPlatform::~VulkanPlatform()
    {
        shutdown();
    }

    PlatformKind VulkanPlatform::kind() const noexcept
    {
        return PlatformKind::Vulkan;
    }

    std::string_view VulkanPlatform::name() const noexcept
    {
        return "Vulkan";
    }

    bool VulkanPlatform::initialize(const RenderingSettings &settings)
    {
        if (m_initialized)
            return true;

        if (!createInstance(settings) || !selectPhysicalDevice() || !createDevice())
            return false;

        createAllocator();
        createBindlessTable();
        createFrames();
        openPipelineCache(settings);
        m_command_list = std::make_unique<vulkan::VulkanCommandList>(*this);

#if defined(ATMO_PROFILER_TRACY)
        if (m_properties.limits.timestampComputeAndGraphics && !m_portability_subset) {
            VkCommandBufferAllocateInfo allocate{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                  .commandPool = m_frames[0].pool,
                                                  .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                  .commandBufferCount = 1 };
            VkCommandBuffer commands = VK_NULL_HANDLE;
            Check(vkAllocateCommandBuffers(m_device, &allocate, &commands), "vkAllocateCommandBuffers");
            m_profiler.context = tracy::CreateVkContext(m_physical_device, m_device, m_queue, commands, nullptr, nullptr);
            vkFreeCommandBuffers(m_device, m_frames[0].pool, 1, &commands);
            if (m_profiler.context)
                m_profiler.context->Name("Vulkan", 6);
        }
#endif

        m_caps.device_name = m_properties.deviceName;
        m_caps.max_texture_size = m_properties.limits.maxImageDimension2D;
        m_caps.max_push_constant_size = push_constant_size;
        m_caps.frames_in_flight = frames_in_flight;
        m_caps.software_device = m_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
        m_initialized = true;

        spdlog::info(
            "Vulkan {} on {} (driver {}){}",
            VersionString(m_properties.apiVersion),
            m_properties.deviceName,
            VersionString(m_properties.driverVersion),
            m_validation ? ", validation enabled" : "");
        return true;
    }

    bool VulkanPlatform::createInstance(const RenderingSettings &settings)
    {
        if (!SDL_WasInit(SDL_INIT_VIDEO)) {
            spdlog::error("The Vulkan platform needs the SDL video subsystem");
            return false;
        }

        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
#if defined(__APPLE__)
            const char *base_path = SDL_GetBasePath();
            const std::string bundled = std::string(base_path ? base_path : "") + "libMoltenVK.dylib";
            if (!SDL_Vulkan_LoadLibrary(bundled.c_str())) {
                spdlog::error("Failed to load a Vulkan library or {}: {}", bundled, SDL_GetError());
                return false;
            }
#else
            spdlog::error("Failed to load the Vulkan library: {}", SDL_GetError());
            return false;
#endif
        }
        m_sdl_library_loaded = true;

        volkInitializeCustom(reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()));

        const std::uint32_t instance_version = volkGetInstanceVersion();
        if (instance_version < VK_API_VERSION_1_3) {
            spdlog::error("Vulkan 1.3 is required, the instance only supports {}", VersionString(instance_version));
            return false;
        }

        std::uint32_t count = 0;
        Check(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr), "vkEnumerateInstanceExtensionProperties");
        std::vector<VkExtensionProperties> available(count);
        Check(vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data()), "vkEnumerateInstanceExtensionProperties");

        Check(vkEnumerateInstanceLayerProperties(&count, nullptr), "vkEnumerateInstanceLayerProperties");
        std::vector<VkLayerProperties> layers_available(count);
        Check(vkEnumerateInstanceLayerProperties(&count, layers_available.data()), "vkEnumerateInstanceLayerProperties");

        std::vector<const char *> extensions;
        std::vector<const char *> layers;

        Uint32 sdl_count = 0;
        const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_count);
        m_caps.presentation = sdl_extensions && sdl_count > 0;
        for (Uint32 i = 0; i < sdl_count; ++i) {
            if (Contains(available, sdl_extensions[i])) {
                AddUnique(extensions, sdl_extensions[i]);
            } else {
                spdlog::warn("Vulkan instance extension {} is unavailable, windows cannot be presented", sdl_extensions[i]);
                m_caps.presentation = false;
            }
        }

        VkInstanceCreateFlags flags = 0;
        if (Contains(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
            AddUnique(extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }

        m_debug_utils = Contains(available, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        if (m_debug_utils)
            AddUnique(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        if (settings.gpu_validation) {
            if (Contains(layers_available, validation_layer)) {
                layers.push_back(validation_layer);
                m_validation = true;
            } else {
                spdlog::warn("GPU validation was requested but {} is not installed", validation_layer);
            }
        }

        const VkApplicationInfo application{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Atmo",
            .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
            .pEngineName = "Atmo",
            .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
            .apiVersion = VK_API_VERSION_1_3,
        };

        const VkDebugUtilsMessengerCreateInfoEXT messenger{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = DebugCallback,
            .pUserData = this,
        };

        const VkInstanceCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = m_validation && m_debug_utils ? &messenger : nullptr,
            .flags = flags,
            .pApplicationInfo = &application,
            .enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        Check(vkCreateInstance(&info, nullptr, &m_instance), "vkCreateInstance");
        volkLoadInstanceOnly(m_instance);

        if (m_validation && m_debug_utils)
            Check(vkCreateDebugUtilsMessengerEXT(m_instance, &messenger, nullptr, &m_messenger), "vkCreateDebugUtilsMessengerEXT");

        return true;
    }

    bool VulkanPlatform::selectPhysicalDevice()
    {
        std::uint32_t count = 0;
        Check(vkEnumeratePhysicalDevices(m_instance, &count, nullptr), "vkEnumeratePhysicalDevices");
        std::vector<VkPhysicalDevice> devices(count);
        Check(vkEnumeratePhysicalDevices(m_instance, &count, devices.data()), "vkEnumeratePhysicalDevices");

        int best_score = -1;
        for (VkPhysicalDevice device : devices) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(device, &properties);

            if (properties.apiVersion < VK_API_VERSION_1_3) {
                spdlog::debug("Skipping {}: Vulkan {} is older than 1.3", properties.deviceName, VersionString(properties.apiVersion));
                continue;
            }

            VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
            VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &features13 };
            VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, .pNext = &features12 };
            VkPhysicalDeviceFeatures2 features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &features11 };
            vkGetPhysicalDeviceFeatures2(device, &features);

            const bool supported = features11.shaderDrawParameters && features12.runtimeDescriptorArray && features12.descriptorBindingPartiallyBound &&
                features12.descriptorBindingSampledImageUpdateAfterBind && features12.descriptorBindingStorageBufferUpdateAfterBind &&
                features12.shaderSampledImageArrayNonUniformIndexing && features13.dynamicRendering && features13.synchronization2;
            if (!supported) {
                spdlog::debug("Skipping {}: missing dynamic rendering, synchronization2 or descriptor indexing", properties.deviceName);
                continue;
            }

            std::uint32_t extension_count = 0;
            Check(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr), "vkEnumerateDeviceExtensionProperties");
            std::vector<VkExtensionProperties> extensions(extension_count);
            Check(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data()), "vkEnumerateDeviceExtensionProperties");
            if (m_caps.presentation && !Contains(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
                spdlog::debug("Skipping {}: no swapchain support", properties.deviceName);
                continue;
            }

            std::uint32_t family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
            std::vector<VkQueueFamilyProperties> families(family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families.data());
            auto family = std::ranges::find_if(families, [](const VkQueueFamilyProperties &f) { return (f.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0; });
            if (family == families.end())
                continue;

            int score = 0;
            switch (properties.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    score = 4;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score = 3;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    score = 2;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    score = 1;
                    break;
                default:
                    break;
            }

            if (score > best_score) {
                best_score = score;
                m_physical_device = device;
                m_properties = properties;
                m_queue_family = static_cast<std::uint32_t>(std::distance(families.begin(), family));
                m_portability_subset = Contains(extensions, portability_subset_extension);
            }
        }

        if (!m_physical_device) {
            spdlog::error("No GPU supports Vulkan 1.3 with dynamic rendering, synchronization2 and descriptor indexing");
            return false;
        }
        return true;
    }

    bool VulkanPlatform::createDevice()
    {
        const float priority = 1.0f;
        const VkDeviceQueueCreateInfo queue{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = m_queue_family,
            .queueCount = 1,
            .pQueuePriorities = &priority,
        };

        std::vector<const char *> extensions;
        if (m_caps.presentation)
            extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        if (m_portability_subset)
            extensions.push_back(portability_subset_extension);

        VkPhysicalDeviceVulkan13Features features13{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE,
        };
        VkPhysicalDeviceVulkan12Features features12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = &features13,
            .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
            .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
            .descriptorBindingPartiallyBound = VK_TRUE,
            .runtimeDescriptorArray = VK_TRUE,
        };
        VkPhysicalDeviceVulkan11Features features11{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
            .pNext = &features12,
            .shaderDrawParameters = VK_TRUE,
        };
        const VkPhysicalDeviceFeatures2 features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &features11 };

        const VkDeviceCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &features,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue,
            .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        Check(vkCreateDevice(m_physical_device, &info, nullptr, &m_device), "vkCreateDevice");
        volkLoadDevice(m_device);
        vkGetDeviceQueue(m_device, m_queue_family, 0, &m_queue);
        return true;
    }

    void VulkanPlatform::createAllocator()
    {
        const VmaVulkanFunctions functions{
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        };
        const VmaAllocatorCreateInfo info{
            .physicalDevice = m_physical_device,
            .device = m_device,
            .pVulkanFunctions = &functions,
            .instance = m_instance,
            .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        Check(vmaCreateAllocator(&info, &m_allocator), "vmaCreateAllocator");
    }

    void VulkanPlatform::createBindlessTable()
    {
        VkPhysicalDeviceVulkan12Properties properties12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
        VkPhysicalDeviceProperties2 properties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &properties12 };
        vkGetPhysicalDeviceProperties2(m_physical_device, &properties);

        const std::uint32_t textures = std::min(
            { max_bindless_textures,
              properties12.maxDescriptorSetUpdateAfterBindSampledImages,
              properties12.maxPerStageDescriptorUpdateAfterBindSampledImages });
        const std::uint32_t samplers = std::min(
            { max_bindless_samplers, properties12.maxDescriptorSetUpdateAfterBindSamplers, properties12.maxPerStageDescriptorUpdateAfterBindSamplers });
        const std::uint32_t buffers = std::min(
            { max_bindless_buffers,
              properties12.maxDescriptorSetUpdateAfterBindStorageBuffers,
              properties12.maxPerStageDescriptorUpdateAfterBindStorageBuffers });

        const std::array bindings{
            VkDescriptorSetLayoutBinding{
                .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = textures, .stageFlags = VK_SHADER_STAGE_ALL },
            VkDescriptorSetLayoutBinding{
                .binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = samplers, .stageFlags = VK_SHADER_STAGE_ALL },
            VkDescriptorSetLayoutBinding{
                .binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = buffers, .stageFlags = VK_SHADER_STAGE_ALL },
        };
        const std::array<VkDescriptorBindingFlags, 3> binding_flags{
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        };
        const VkDescriptorSetLayoutBindingFlagsCreateInfo flags_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = static_cast<std::uint32_t>(binding_flags.size()),
            .pBindingFlags = binding_flags.data(),
        };
        const VkDescriptorSetLayoutCreateInfo layout_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &flags_info,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = static_cast<std::uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };
        Check(vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_bindless_layout), "vkCreateDescriptorSetLayout");

        const std::array pool_sizes{
            VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = textures },
            VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = samplers },
            VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = buffers },
        };
        const VkDescriptorPoolCreateInfo pool_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
            .maxSets = 1,
            .poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };
        Check(vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_bindless_pool), "vkCreateDescriptorPool");

        const VkDescriptorSetAllocateInfo set_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_bindless_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_bindless_layout,
        };
        Check(vkAllocateDescriptorSets(m_device, &set_info, &m_bindless_set), "vkAllocateDescriptorSets");

        const VkPushConstantRange push_constants{ .stageFlags = VK_SHADER_STAGE_ALL, .offset = 0, .size = push_constant_size };
        const VkPipelineLayoutCreateInfo pipeline_layout_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_bindless_layout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &push_constants,
        };
        Check(vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout), "vkCreatePipelineLayout");

        m_free_texture_indices.clear();
        for (std::uint32_t i = textures; i > 0; --i) m_free_texture_indices.push_back(i - 1);
        m_free_sampler_indices.clear();
        for (std::uint32_t i = samplers; i > 0; --i) m_free_sampler_indices.push_back(i - 1);
        m_free_buffer_indices.clear();
        for (std::uint32_t i = buffers; i > 0; --i) m_free_buffer_indices.push_back(i - 1);

        m_caps.max_bindless_textures = textures;
        m_caps.max_bindless_samplers = samplers;
        m_caps.max_bindless_buffers = buffers;
    }

    void VulkanPlatform::createFrames()
    {
        for (Frame &frame : m_frames) {
            const VkCommandPoolCreateInfo pool_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = m_queue_family,
            };
            Check(vkCreateCommandPool(m_device, &pool_info, nullptr, &frame.pool), "vkCreateCommandPool");

            std::array<VkCommandBuffer, 2> buffers{};
            const VkCommandBufferAllocateInfo allocate{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = frame.pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = static_cast<std::uint32_t>(buffers.size()),
            };
            Check(vkAllocateCommandBuffers(m_device, &allocate, buffers.data()), "vkAllocateCommandBuffers");
            frame.commands = buffers[0];
            frame.uploads = buffers[1];

            const VkFenceCreateInfo fence_info{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
            Check(vkCreateFence(m_device, &fence_info, nullptr, &frame.fence), "vkCreateFence");

            const VkBufferCreateInfo staging_info{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = staging_size,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            };
            const VmaAllocationCreateInfo allocation_info{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO,
            };
            VmaAllocationInfo allocated{};
            Check(vmaCreateBuffer(m_allocator, &staging_info, &allocation_info, &frame.staging, &frame.staging_allocation, &allocated), "vmaCreateBuffer");
            frame.staging_mapped = static_cast<std::byte *>(allocated.pMappedData);
            setDebugName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<std::uint64_t>(frame.staging), "staging ring");
        }
    }

    void VulkanPlatform::destroyFrames() noexcept
    {
        for (Frame &frame : m_frames) {
            if (frame.staging)
                vmaDestroyBuffer(m_allocator, frame.staging, frame.staging_allocation);
            if (frame.fence)
                vkDestroyFence(m_device, frame.fence, nullptr);
            if (frame.pool)
                vkDestroyCommandPool(m_device, frame.pool, nullptr);
            frame = Frame{};
        }
    }

    void VulkanPlatform::openPipelineCache(const RenderingSettings &settings)
    {
        std::vector<char> data;
        m_pipeline_cache_loaded = false;
        if (!settings.pipeline_cache_dir.empty()) {
            std::error_code error;
            std::filesystem::create_directories(settings.pipeline_cache_dir, error);
            m_pipeline_cache_path = std::filesystem::path(settings.pipeline_cache_dir) / pipeline_cache_file;

            std::ifstream in(m_pipeline_cache_path, std::ios::binary);
            if (in.is_open())
                data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());

            VkPipelineCacheHeaderVersionOne header{};
            if (data.size() >= sizeof(header))
                std::memcpy(&header, data.data(), sizeof(header));
            const bool compatible = data.size() >= sizeof(header) && header.headerSize >= sizeof(header) &&
                header.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE && header.vendorID == m_properties.vendorID &&
                header.deviceID == m_properties.deviceID && std::memcmp(header.pipelineCacheUUID, m_properties.pipelineCacheUUID, VK_UUID_SIZE) == 0;
            if (!data.empty() && !compatible) {
                spdlog::info("Discarding the Vulkan pipeline cache {}: it was written by another device or driver", m_pipeline_cache_path.string());
                data.clear();
            }
        }

        const VkPipelineCacheCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
            .initialDataSize = data.size(),
            .pInitialData = data.empty() ? nullptr : data.data(),
        };
        if (vkCreatePipelineCache(m_device, &info, nullptr, &m_pipeline_cache) != VK_SUCCESS) {
            const VkPipelineCacheCreateInfo empty{ .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
            Check(vkCreatePipelineCache(m_device, &empty, nullptr, &m_pipeline_cache), "vkCreatePipelineCache");
            data.clear();
        }
        m_pipeline_cache_loaded = !data.empty();
    }

    void VulkanPlatform::savePipelineCache() noexcept
    {
        if (!m_pipeline_cache || m_pipeline_cache_path.empty())
            return;

        std::size_t size = 0;
        if (vkGetPipelineCacheData(m_device, m_pipeline_cache, &size, nullptr) != VK_SUCCESS || size == 0)
            return;
        std::vector<char> data(size);
        if (vkGetPipelineCacheData(m_device, m_pipeline_cache, &size, data.data()) != VK_SUCCESS)
            return;

        std::ofstream out(m_pipeline_cache_path, std::ios::binary | std::ios::trunc);
        out.write(data.data(), static_cast<std::streamsize>(size));
        if (!out.good())
            spdlog::warn("Failed to save the Vulkan pipeline cache {}", m_pipeline_cache_path.string());
    }

    const std::filesystem::path &VulkanPlatform::pipelineCachePath() const noexcept
    {
        return m_pipeline_cache_path;
    }

    bool VulkanPlatform::pipelineCacheLoaded() const noexcept
    {
        return m_pipeline_cache_loaded;
    }

    void VulkanPlatform::shutdown()
    {
        if (m_device) {
            vkDeviceWaitIdle(m_device);

            m_swapchains.forEach([this](SwapchainHandle, Swapchain &swapchain) {
                swapchainReleaseImages(swapchain);
                for (VkSemaphore semaphore : swapchain.acquire_semaphores)
                    if (semaphore)
                        vkDestroySemaphore(m_device, semaphore, nullptr);
                if (swapchain.swapchain)
                    vkDestroySwapchainKHR(m_device, swapchain.swapchain, nullptr);
                surfaceDestroy(swapchain);
            });
            m_swapchains.clear();

            const std::size_t leaked = m_pipelines.size() + m_modules.size() + m_samplers.size() + m_textures.size() + m_buffers.size();
            if (leaked > 0)
                spdlog::debug("Destroying {} Vulkan objects that were never freed", leaked);

            m_pipelines.forEach([this](PipelineHandle, VkPipeline &pipeline) { vkDestroyPipeline(m_device, pipeline, nullptr); });
            m_modules.forEach([this](ShaderModuleHandle, VkShaderModule &module) { vkDestroyShaderModule(m_device, module, nullptr); });
            m_samplers.forEach([this](SamplerHandle, Sampler &sampler) { vkDestroySampler(m_device, sampler.sampler, nullptr); });
            m_textures.forEach([this](TextureHandle, Texture &texture) {
                vkDestroyImageView(m_device, texture.view, nullptr);
                if (texture.allocation)
                    vmaDestroyImage(m_allocator, texture.image, texture.allocation);
            });
            m_buffers.forEach([this](BufferHandle, Buffer &buffer) { vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation); });
            m_pipelines.clear();
            m_modules.clear();
            m_samplers.clear();
            savePipelineCache();
            if (m_pipeline_cache)
                vkDestroyPipelineCache(m_device, m_pipeline_cache, nullptr);
            m_pipeline_cache = VK_NULL_HANDLE;
            m_textures.clear();
            m_buffers.clear();

            collectGarbage(~0ull);

#if defined(ATMO_PROFILER_TRACY)
            if (m_profiler.context)
                tracy::DestroyVkContext(m_profiler.context);
#endif
            m_profiler = {};
            m_command_list.reset();
            destroyFrames();

            if (m_pipeline_layout)
                vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
            if (m_bindless_pool)
                vkDestroyDescriptorPool(m_device, m_bindless_pool, nullptr);
            if (m_bindless_layout)
                vkDestroyDescriptorSetLayout(m_device, m_bindless_layout, nullptr);
            if (m_allocator)
                vmaDestroyAllocator(m_allocator);
            vkDestroyDevice(m_device, nullptr);
        }

        if (m_messenger)
            vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
        if (m_instance)
            vkDestroyInstance(m_instance, nullptr);
        if (m_sdl_library_loaded) {
            SDL_Vulkan_UnloadLibrary();
            volkFinalize();
        }

        m_device = VK_NULL_HANDLE;
        m_instance = VK_NULL_HANDLE;
        m_messenger = VK_NULL_HANDLE;
        m_physical_device = VK_NULL_HANDLE;
        m_queue = VK_NULL_HANDLE;
        m_allocator = VK_NULL_HANDLE;
        m_pipeline_layout = VK_NULL_HANDLE;
        m_bindless_pool = VK_NULL_HANDLE;
        m_bindless_layout = VK_NULL_HANDLE;
        m_bindless_set = VK_NULL_HANDLE;
        m_deletions.clear();
        m_frame_number = 0;
        m_completed_frame = 0;
        m_in_frame = false;
        m_submitted = false;
        m_sdl_library_loaded = false;
        m_validation = false;
        m_debug_utils = false;
        m_initialized = false;
    }

    SDL_WindowFlags VulkanPlatform::windowFlags() const noexcept
    {
        return SDL_WINDOW_VULKAN;
    }

    const PlatformCaps &VulkanPlatform::caps() const noexcept
    {
        return m_caps;
    }

    ShaderTarget VulkanPlatform::shaderTarget() const noexcept
    {
        return ShaderTarget::SpirV;
    }

    VulkanPlatform::Frame &VulkanPlatform::prepareFrame(std::uint64_t number)
    {
        Frame &frame = m_frames[number % frames_in_flight];
        if (frame.number == number)
            return frame;

        Check(vkWaitForFences(m_device, 1, &frame.fence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
        if (number > frames_in_flight)
            m_completed_frame = std::max(m_completed_frame, number - frames_in_flight);
        collectGarbage(m_completed_frame);

        Check(vkResetCommandPool(m_device, frame.pool, 0), "vkResetCommandPool");
        frame.staging_offset = 0;
        frame.uploads_recording = false;
        frame.number = number;
        return frame;
    }

    VkCommandBuffer VulkanPlatform::transferCommands()
    {
        if (m_in_frame) {
            if (m_submitted)
                throw std::logic_error("GPU transfers are not allowed after the frame was submitted");
            if (m_command_list->inRenderPass())
                throw std::logic_error("GPU transfers are not allowed inside a render pass");
            return m_command_list->commands();
        }

        Frame &frame = prepareFrame(m_frame_number + 1);
        if (!frame.uploads_recording) {
            const VkCommandBufferBeginInfo begin{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
            Check(vkBeginCommandBuffer(frame.uploads, &begin), "vkBeginCommandBuffer");
            frame.uploads_recording = true;
        }
        return frame.uploads;
    }

    VulkanPlatform::StagingSpan VulkanPlatform::stagingAllocate(VkDeviceSize size)
    {
        Frame &frame = m_in_frame ? m_frames[m_frame_number % frames_in_flight] : prepareFrame(m_frame_number + 1);

        const VkDeviceSize offset = AlignUp(frame.staging_offset, 16);
        if (offset + size <= staging_size) {
            frame.staging_offset = offset + size;
            return { frame.staging, offset, frame.staging_mapped + offset };
        }

        const VkBufferCreateInfo info{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
        const VmaAllocationCreateInfo allocation_info{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocated{};
        Check(vmaCreateBuffer(m_allocator, &info, &allocation_info, &buffer, &allocation, &allocated), "vmaCreateBuffer");
        defer([this, buffer, allocation] { vmaDestroyBuffer(m_allocator, buffer, allocation); });
        return { buffer, 0, static_cast<std::byte *>(allocated.pMappedData) };
    }

    void VulkanPlatform::defer(std::function<void()> destroy)
    {
        m_deletions.push_back({ .frame = m_in_frame ? m_frame_number : m_frame_number + 1, .destroy = std::move(destroy) });
    }

    void VulkanPlatform::collectGarbage(std::uint64_t completed_frame) noexcept
    {
        auto ready = [completed_frame](const Deletion &deletion) { return deletion.frame <= completed_frame; };
        std::vector<Deletion> remaining;
        std::vector<Deletion> due;
        for (Deletion &deletion : m_deletions) (ready(deletion) ? due : remaining).push_back(std::move(deletion));
        m_deletions = std::move(remaining);
        for (Deletion &deletion : due) deletion.destroy();
    }

    void VulkanPlatform::waitIdle() noexcept
    {
        vkDeviceWaitIdle(m_device);
        m_completed_frame = std::max(m_completed_frame, m_frame_number);
        collectGarbage(m_completed_frame);
    }

    void VulkanPlatform::setDebugName(VkObjectType type, std::uint64_t handle, std::string_view name) const
    {
        if (!m_debug_utils || name.empty())
            return;

        const std::string owned(name);
        const VkDebugUtilsObjectNameInfoEXT info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = handle,
            .pObjectName = owned.c_str(),
        };
        vkSetDebugUtilsObjectNameEXT(m_device, &info);
    }

    void VulkanPlatform::transition(VkCommandBuffer commands, Texture &texture, ResourceState after) const
    {
        const bool read_only = after == ResourceState::ShaderRead || after == ResourceState::CopySrc || after == ResourceState::Present;
        if (texture.state == after && read_only)
            return;

        const vulkan::StateInfo source = vulkan::SourceState(texture.state);
        const vulkan::StateInfo destination = vulkan::DestinationState(after);
        const VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = source.stages,
            .srcAccessMask = source.access,
            .dstStageMask = destination.stages,
            .dstAccessMask = destination.access,
            .oldLayout = source.layout,
            .newLayout = destination.layout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = texture.image,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        };
        const VkDependencyInfo dependency{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier };
        vkCmdPipelineBarrier2(commands, &dependency);
        texture.state = after;
    }

    VulkanPlatform::Texture &VulkanPlatform::texture(TextureHandle handle)
    {
        return m_textures.at(handle);
    }

    const VulkanPlatform::Texture &VulkanPlatform::texture(TextureHandle handle) const
    {
        return m_textures.at(handle);
    }

    BufferHandle VulkanPlatform::bufferCreate(const BufferDesc &desc)
    {
        if (desc.size == 0)
            throw std::invalid_argument("bufferCreate: size must not be 0");
        const VkBufferUsageFlags usage = vulkan::ToVkBufferUsage(desc.usage);
        if (usage == 0)
            throw std::invalid_argument("bufferCreate: no usage given");
        const bool storage = HasFlag(desc.usage, BufferUsage::Storage);
        if (storage && m_free_buffer_indices.empty())
            throw std::length_error("bufferCreate: the bindless buffer table is full");

        const VkBufferCreateInfo info{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = desc.size, .usage = usage };
        VmaAllocationCreateInfo allocation_info{ .usage = VMA_MEMORY_USAGE_AUTO };
        if (desc.memory == MemoryType::Upload)
            allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        else if (desc.memory == MemoryType::Readback)
            allocation_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

        Buffer buffer{ .desc = desc };
        buffer.desc.debug_name = {};
        VmaAllocationInfo allocated{};
        Check(vmaCreateBuffer(m_allocator, &info, &allocation_info, &buffer.buffer, &buffer.allocation, &allocated), "vmaCreateBuffer");
        buffer.mapped = allocated.pMappedData;
        setDebugName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<std::uint64_t>(buffer.buffer), desc.debug_name);

        if (storage) {
            buffer.bindless = m_free_buffer_indices.back();
            m_free_buffer_indices.pop_back();

            const VkDescriptorBufferInfo info{ .buffer = buffer.buffer, .offset = 0, .range = VK_WHOLE_SIZE };
            const VkWriteDescriptorSet write{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_bindless_set,
                .dstBinding = 2,
                .dstArrayElement = buffer.bindless,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pBufferInfo = &info,
            };
            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }

        return m_buffers.insert(buffer);
    }

    void VulkanPlatform::bufferWrite(BufferHandle buffer, std::uint64_t offset, std::span<const std::byte> data)
    {
        Buffer &entry = m_buffers.at(buffer);
        if (entry.desc.memory != MemoryType::Upload || !entry.mapped)
            throw std::logic_error("bufferWrite: buffer is not host visible");
        if (offset + data.size() > entry.desc.size)
            throw std::out_of_range("bufferWrite: write past the end of the buffer");

        std::memcpy(static_cast<std::byte *>(entry.mapped) + offset, data.data(), data.size());
        Check(vmaFlushAllocation(m_allocator, entry.allocation, offset, data.size()), "vmaFlushAllocation");
    }

    TextureHandle VulkanPlatform::textureCreate(const TextureDesc &desc)
    {
        if (desc.size.x <= 0 || desc.size.y <= 0 || static_cast<std::uint32_t>(std::max(desc.size.x, desc.size.y)) > m_caps.max_texture_size)
            throw std::invalid_argument("textureCreate: invalid size");
        const VkFormat format = vulkan::ToVkFormat(desc.format);
        if (format == VK_FORMAT_UNDEFINED)
            throw std::invalid_argument("textureCreate: invalid format");
        const VkImageUsageFlags usage = vulkan::ToVkImageUsage(desc.usage);
        if (usage == 0)
            throw std::invalid_argument("textureCreate: no usage given");
        if (HasFlag(desc.usage, TextureUsage::Sampled) && m_free_texture_indices.empty())
            throw std::length_error("textureCreate: the bindless texture table is full");

        const VkImageCreateInfo image_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = { static_cast<std::uint32_t>(desc.size.x), static_cast<std::uint32_t>(desc.size.y), 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        const VmaAllocationCreateInfo allocation_info{ .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };

        Texture texture{ .format = format, .desc = desc };
        texture.desc.debug_name = {};
        Check(vmaCreateImage(m_allocator, &image_info, &allocation_info, &texture.image, &texture.allocation, nullptr), "vmaCreateImage");

        const VkImageViewCreateInfo view_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = texture.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        };
        if (VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &texture.view); result != VK_SUCCESS) {
            vmaDestroyImage(m_allocator, texture.image, texture.allocation);
            throw vulkan::VulkanError(result, "vkCreateImageView");
        }
        setDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<std::uint64_t>(texture.image), desc.debug_name);

        if (HasFlag(desc.usage, TextureUsage::Sampled)) {
            texture.bindless = m_free_texture_indices.back();
            m_free_texture_indices.pop_back();

            const VkDescriptorImageInfo image{ .imageView = texture.view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
            const VkWriteDescriptorSet write{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_bindless_set,
                .dstBinding = 0,
                .dstArrayElement = texture.bindless,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .pImageInfo = &image,
            };
            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }

        return m_textures.insert(texture);
    }

    void VulkanPlatform::textureUpload(TextureHandle handle, const Region2i &region, std::span<const std::byte> pixels)
    {
        Texture &entry = texture(handle);
        if (!HasFlag(entry.desc.usage, TextureUsage::CopyDst))
            throw std::logic_error("textureUpload: texture was not created with CopyDst usage");
        if (region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 || region.x + region.width > entry.desc.size.x ||
            region.y + region.height > entry.desc.size.y)
            throw std::out_of_range("textureUpload: region outside the texture");

        const VkDeviceSize size = static_cast<VkDeviceSize>(region.width) * static_cast<VkDeviceSize>(region.height) * BytesPerPixel(entry.desc.format);
        if (pixels.size() != size)
            throw std::invalid_argument("textureUpload: pixel data does not match the region");

        VkCommandBuffer commands = transferCommands();
        const StagingSpan staging = stagingAllocate(size);
        std::memcpy(staging.data, pixels.data(), pixels.size());

        transition(commands, entry, ResourceState::CopyDst);
        const VkBufferImageCopy copy{
            .bufferOffset = staging.offset,
            .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
            .imageOffset = { region.x, region.y, 0 },
            .imageExtent = { static_cast<std::uint32_t>(region.width), static_cast<std::uint32_t>(region.height), 1 },
        };
        vkCmdCopyBufferToImage(commands, staging.buffer, entry.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        if (HasFlag(entry.desc.usage, TextureUsage::Sampled))
            transition(commands, entry, ResourceState::ShaderRead);
    }

    std::vector<std::byte> VulkanPlatform::textureReadback(TextureHandle handle)
    {
        if (m_in_frame)
            throw std::logic_error("textureReadback called during a frame");

        Texture &entry = texture(handle);
        if (!HasFlag(entry.desc.usage, TextureUsage::CopySrc))
            throw std::logic_error("textureReadback: texture was not created with CopySrc usage");

        const VkDeviceSize size =
            static_cast<VkDeviceSize>(entry.desc.size.x) * static_cast<VkDeviceSize>(entry.desc.size.y) * BytesPerPixel(entry.desc.format);
        const VkBufferCreateInfo info{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT };
        const VmaAllocationCreateInfo allocation_info{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VmaAllocationInfo allocated{};
        Check(vmaCreateBuffer(m_allocator, &info, &allocation_info, &buffer, &allocation, &allocated), "vmaCreateBuffer");

        std::vector<std::byte> pixels(size);
        try {
            VkCommandBuffer commands = transferCommands();
            const ResourceState previous = entry.state;
            transition(commands, entry, ResourceState::CopySrc);
            const VkBufferImageCopy copy{
                .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
                .imageExtent = { static_cast<std::uint32_t>(entry.desc.size.x), static_cast<std::uint32_t>(entry.desc.size.y), 1 },
            };
            vkCmdCopyImageToBuffer(commands, entry.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &copy);
            if (previous != ResourceState::Undefined)
                transition(commands, entry, previous);

            Frame &frame = m_frames[(m_frame_number + 1) % frames_in_flight];
            Check(vkEndCommandBuffer(frame.uploads), "vkEndCommandBuffer");
            frame.uploads_recording = false;

            const VkCommandBufferSubmitInfo buffer_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = frame.uploads };
            const VkSubmitInfo2 submit{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &buffer_info };
            Check(vkQueueSubmit2(m_queue, 1, &submit, VK_NULL_HANDLE), "vkQueueSubmit2");
            Check(vkQueueWaitIdle(m_queue), "vkQueueWaitIdle");

            Check(vmaInvalidateAllocation(m_allocator, allocation, 0, size), "vmaInvalidateAllocation");
            std::memcpy(pixels.data(), allocated.pMappedData, size);
        } catch (...) {
            vmaDestroyBuffer(m_allocator, buffer, allocation);
            throw;
        }

        vmaDestroyBuffer(m_allocator, buffer, allocation);
        m_completed_frame = std::max(m_completed_frame, m_frame_number);
        collectGarbage(m_completed_frame);
        return pixels;
    }

    core::types::Vector2i VulkanPlatform::textureSize(TextureHandle handle) const
    {
        return texture(handle).desc.size;
    }

    SamplerHandle VulkanPlatform::samplerCreate(const SamplerDesc &desc)
    {
        if (m_free_sampler_indices.empty())
            throw std::length_error("samplerCreate: the bindless sampler table is full");

        const VkSamplerAddressMode address = vulkan::ToVkAddressMode(desc.address);
        const VkSamplerCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = vulkan::ToVkFilter(desc.filter),
            .minFilter = vulkan::ToVkFilter(desc.filter),
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = address,
            .addressModeV = address,
            .addressModeW = address,
            .maxLod = VK_LOD_CLAMP_NONE,
        };

        Sampler sampler;
        Check(vkCreateSampler(m_device, &info, nullptr, &sampler.sampler), "vkCreateSampler");
        sampler.bindless = m_free_sampler_indices.back();
        m_free_sampler_indices.pop_back();

        const VkDescriptorImageInfo image{ .sampler = sampler.sampler };
        const VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_bindless_set,
            .dstBinding = 1,
            .dstArrayElement = sampler.bindless,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &image,
        };
        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

        return m_samplers.insert(sampler);
    }

    ShaderModuleHandle VulkanPlatform::shaderModuleCreate(const ShaderBlob &blob)
    {
        if (blob.target != ShaderTarget::SpirV)
            throw std::invalid_argument("shaderModuleCreate: the Vulkan platform only accepts SPIR-V");
        if (blob.code.empty() || blob.code.size() % sizeof(std::uint32_t) != 0)
            throw std::invalid_argument("shaderModuleCreate: SPIR-V code size must be a non-zero multiple of 4");

        std::vector<std::uint32_t> words(blob.code.size() / sizeof(std::uint32_t));
        std::memcpy(words.data(), blob.code.data(), blob.code.size());

        const VkShaderModuleCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = blob.code.size(),
            .pCode = words.data(),
        };
        VkShaderModule module = VK_NULL_HANDLE;
        Check(vkCreateShaderModule(m_device, &info, nullptr, &module), "vkCreateShaderModule");
        setDebugName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<std::uint64_t>(module), blob.debug_name);
        return m_modules.insert(module);
    }

    PipelineHandle VulkanPlatform::graphicsPipelineCreate(const GraphicsPipelineDesc &desc)
    {
        const VkShaderModule *vertex = m_modules.get(desc.vertex.module);
        const VkShaderModule *fragment = m_modules.get(desc.fragment.module);
        if (!vertex || !fragment)
            throw std::invalid_argument("graphicsPipelineCreate: unknown shader module");
        const VkFormat color_format = vulkan::ToVkFormat(desc.color_format);
        if (color_format == VK_FORMAT_UNDEFINED)
            throw std::invalid_argument("graphicsPipelineCreate: color format is undefined");

        const std::string vertex_entry(desc.vertex.entry_point);
        const std::string fragment_entry(desc.fragment.entry_point);
        const std::array stages{
            VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = *vertex,
                .pName = vertex_entry.c_str(),
            },
            VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = *fragment,
                .pName = fragment_entry.c_str(),
            },
        };

        const VkPipelineVertexInputStateCreateInfo vertex_input{ .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        const VkPipelineInputAssemblyStateCreateInfo input_assembly{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = vulkan::ToVkTopology(desc.topology),
        };
        const VkPipelineViewportStateCreateInfo viewport{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
        };
        const VkPipelineRasterizationStateCreateInfo rasterization{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.0f,
        };
        const VkPipelineMultisampleStateCreateInfo multisample{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };
        const VkPipelineColorBlendAttachmentState blend_attachment = vulkan::ToVkBlend(desc.blend);
        const VkPipelineColorBlendStateCreateInfo blend{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &blend_attachment,
        };
        const std::array dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        const VkPipelineDynamicStateCreateInfo dynamic{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data(),
        };
        const VkPipelineRenderingCreateInfo rendering{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &color_format,
        };

        const VkGraphicsPipelineCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering,
            .stageCount = static_cast<std::uint32_t>(stages.size()),
            .pStages = stages.data(),
            .pVertexInputState = &vertex_input,
            .pInputAssemblyState = &input_assembly,
            .pViewportState = &viewport,
            .pRasterizationState = &rasterization,
            .pMultisampleState = &multisample,
            .pColorBlendState = &blend,
            .pDynamicState = &dynamic,
            .layout = m_pipeline_layout,
        };

        VkPipeline pipeline = VK_NULL_HANDLE;
        Check(vkCreateGraphicsPipelines(m_device, m_pipeline_cache, 1, &info, nullptr, &pipeline), "vkCreateGraphicsPipelines");
        setDebugName(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<std::uint64_t>(pipeline), desc.debug_name);
        return m_pipelines.insert(pipeline);
    }

    void VulkanPlatform::destroy(BufferHandle buffer)
    {
        std::optional<Buffer> entry = m_buffers.remove(buffer);
        if (!entry)
            throw std::invalid_argument("destroy: unknown buffer");
        defer([this, entry = *entry] {
            vmaDestroyBuffer(m_allocator, entry.buffer, entry.allocation);
            if (entry.bindless != invalid_index)
                m_free_buffer_indices.push_back(entry.bindless);
        });
    }

    void VulkanPlatform::destroy(TextureHandle handle)
    {
        const Texture *entry = m_textures.get(handle);
        if (!entry || entry->swapchain_image)
            throw std::invalid_argument("destroy: unknown texture");

        Texture removed = *m_textures.remove(handle);
        defer([this, removed] {
            vkDestroyImageView(m_device, removed.view, nullptr);
            vmaDestroyImage(m_allocator, removed.image, removed.allocation);
            if (removed.bindless != invalid_index)
                m_free_texture_indices.push_back(removed.bindless);
        });
    }

    void VulkanPlatform::destroy(SamplerHandle sampler)
    {
        std::optional<Sampler> entry = m_samplers.remove(sampler);
        if (!entry)
            throw std::invalid_argument("destroy: unknown sampler");
        defer([this, entry = *entry] {
            vkDestroySampler(m_device, entry.sampler, nullptr);
            m_free_sampler_indices.push_back(entry.bindless);
        });
    }

    void VulkanPlatform::destroy(ShaderModuleHandle module)
    {
        std::optional<VkShaderModule> entry = m_modules.remove(module);
        if (!entry)
            throw std::invalid_argument("destroy: unknown shader module");
        defer([this, entry = *entry] { vkDestroyShaderModule(m_device, entry, nullptr); });
    }

    void VulkanPlatform::destroy(PipelineHandle pipeline)
    {
        std::optional<VkPipeline> entry = m_pipelines.remove(pipeline);
        if (!entry)
            throw std::invalid_argument("destroy: unknown pipeline");
        defer([this, entry = *entry] { vkDestroyPipeline(m_device, entry, nullptr); });
    }

    std::uint32_t VulkanPlatform::bindlessIndex(TextureHandle handle) const
    {
        const Texture &entry = texture(handle);
        if (entry.bindless == invalid_index)
            throw std::logic_error("bindlessIndex: texture was not created with Sampled usage");
        return entry.bindless;
    }

    std::uint32_t VulkanPlatform::bindlessIndex(SamplerHandle sampler) const
    {
        return m_samplers.at(sampler).bindless;
    }

    std::uint32_t VulkanPlatform::bindlessIndex(BufferHandle buffer) const
    {
        const Buffer &entry = m_buffers.at(buffer);
        if (entry.bindless == invalid_index)
            throw std::logic_error("bindlessIndex: buffer was not created with Storage usage");
        return entry.bindless;
    }

    void VulkanPlatform::frameBegin()
    {
        if (m_in_frame)
            throw std::logic_error("frameBegin called twice without frameEnd");

        Frame &frame = prepareFrame(m_frame_number + 1);
        m_frame_number = frame.number;
        m_in_frame = true;
        m_submitted = false;
        m_command_list->begin(frame.commands);
    }

    CommandList &VulkanPlatform::commandListAcquire(QueueType queue)
    {
        if (!m_in_frame || m_submitted)
            throw std::logic_error("commandListAcquire called outside a frame or after submit");
        return *m_command_list;
    }

    void VulkanPlatform::submit(std::span<CommandList *const> lists)
    {
        if (!m_in_frame || m_submitted)
            throw std::logic_error("submit called outside a frame or twice in one frame");
        for (CommandList *list : lists)
            if (list != m_command_list.get())
                throw std::invalid_argument("submit: command list does not belong to this frame");
        submitFrame(lists);
    }

    void VulkanPlatform::submitFrame(std::span<CommandList *const> lists)
    {
        if (m_command_list->inRenderPass()) {
            if (!lists.empty())
                throw std::logic_error("submit: a render pass is still open");
            spdlog::error("The frame ended with an open render pass, closing it");
            m_command_list->renderPassEnd();
        }

        Frame &frame = m_frames[m_frame_number % frames_in_flight];
        VkCommandBuffer commands = m_command_list->commands();

        std::vector<VkSemaphoreSubmitInfo> waits;
        std::vector<VkSemaphoreSubmitInfo> signals;
        m_swapchains.forEach([&](SwapchainHandle, Swapchain &swapchain) {
            if (!swapchain.acquired || swapchain.waited)
                return;
            transition(commands, texture(swapchain.images[swapchain.image_index]), ResourceState::Present);
            waits.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .semaphore = swapchain.acquire_semaphores[m_frame_number % frames_in_flight],
                    .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                });
            signals.push_back(
                {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .semaphore = swapchain.present_semaphores[swapchain.image_index],
                    .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                });
            swapchain.waited = true;
        });

#if defined(ATMO_PROFILER_TRACY)
        if (m_profiler.context)
            TracyVkCollect(m_profiler.context, commands);
#endif

        m_command_list->end();

        std::vector<VkCommandBufferSubmitInfo> buffers;
        if (frame.uploads_recording) {
            Check(vkEndCommandBuffer(frame.uploads), "vkEndCommandBuffer");
            frame.uploads_recording = false;
            buffers.push_back({ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = frame.uploads });
        }
        buffers.push_back({ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = commands });

        const VkSubmitInfo2 submit{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = static_cast<std::uint32_t>(waits.size()),
            .pWaitSemaphoreInfos = waits.data(),
            .commandBufferInfoCount = static_cast<std::uint32_t>(buffers.size()),
            .pCommandBufferInfos = buffers.data(),
            .signalSemaphoreInfoCount = static_cast<std::uint32_t>(signals.size()),
            .pSignalSemaphoreInfos = signals.data(),
        };

        Check(vkResetFences(m_device, 1, &frame.fence), "vkResetFences");
        m_submitted = true;
        Check(vkQueueSubmit2(m_queue, 1, &submit, frame.fence), "vkQueueSubmit2");
    }

    void VulkanPlatform::frameEnd()
    {
        if (!m_in_frame)
            throw std::logic_error("frameEnd called without frameBegin");

        try {
            if (!m_submitted)
                submitFrame({});

            std::vector<SwapchainHandle> pending;
            m_swapchains.forEach([&pending](SwapchainHandle handle, Swapchain &swapchain) {
                if (swapchain.acquired && !swapchain.presented)
                    pending.push_back(handle);
            });
            for (SwapchainHandle handle : pending) swapchainPresent(handle);
        } catch (...) {
            m_in_frame = false;
            throw;
        }

        m_in_frame = false;
    }

    bool VulkanPlatform::validationEnabled() const noexcept
    {
        return m_validation;
    }

    std::uint32_t VulkanPlatform::validationErrorCount() const noexcept
    {
        return m_validation_errors;
    }

    std::uint32_t VulkanPlatform::validationWarningCount() const noexcept
    {
        return m_validation_warnings;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanPlatform::DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *data,
        void *user_data)
    {
        auto *platform = static_cast<VulkanPlatform *>(user_data);
        const char *message = data && data->pMessage ? data->pMessage : "";

        if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0) {
            ++platform->m_validation_errors;
            spdlog::error("Vulkan: {}", message);
        } else if ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0) {
            ++platform->m_validation_warnings;
            spdlog::warn("Vulkan: {}", message);
        }
        return VK_FALSE;
    }
} // namespace atmo::server::rendering
