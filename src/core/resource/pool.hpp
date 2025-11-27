#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/resource/handle.hpp"
#include "core/resource/resource.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            class Pool
            {
            public:
                Pool();
                ~Pool();

                const handle create(const std::string &path);
                std::shared_ptr<Resource> getFromHandle(const handle &handle);

                void declareHandle(const handle &handle);
                void destroy(const handle &handle);

            private:
                std::vector<handle> m_usedHandles;
                std::unordered_map<std::string, handle> m_handles;

                std::vector<std::shared_ptr<Resource>> m_resources;
                std::vector<std::uint16_t> m_generations;
                std::vector<std::uint16_t> m_freeList;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
