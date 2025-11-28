#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/resource/resource.hpp"
#include "core/resource/handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class ResourcePool
            {
            public:
                ResourcePool();
                ~ResourcePool();

                ResourcePool(const ResourcePool&) = delete;
                ResourcePool& operator=(const ResourcePool&) = delete;

                ResourcePool(ResourcePool&&) noexcept = default;
                ResourcePool& operator=(ResourcePool&&) noexcept = default;


                const Handle create(const std::string &path);
                std::shared_ptr<Resource> getFromHandle(const Handle &handle);

                void clear();
                void clearHandle(const Handle &handle);
            private:
                void destroy(const Handle &handle);
                std::unordered_map<std::string, Handle> m_handles;

                std::vector<std::shared_ptr<Resource>> m_resources;
                std::vector<std::uint16_t> m_generations;
                std::vector<std::uint16_t> m_freeList;

                std::unique_ptr<std::mutex> m_resourceMutex;
                std::unique_ptr<std::mutex> m_handleManagementMutex;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
