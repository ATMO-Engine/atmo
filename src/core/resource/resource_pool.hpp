#pragma once

#include <cstdint>
#include <memory>
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

                const Handle create(const std::string &path);
                std::shared_ptr<Resource> getFromHandle(const Handle &handle);

                void destroy(const Handle &handle);
                void clear();
            private:
                std::unordered_map<std::string, Handle> m_handles;

                std::vector<std::shared_ptr<Resource>> m_resources;
                std::vector<std::uint16_t> m_generations;
                std::vector<std::uint16_t> m_freeList;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
