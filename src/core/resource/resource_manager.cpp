#include "resource_manager.hpp"
#include "spdlog/spdlog.h"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ResourceManager::ResourceManager()
            {
            }

            ResourceManager &ResourceManager::GetInstance()
            {
                static ResourceManager instance;
                return instance;
            }

            void ResourceManager::registerPool(IPoolGarbageCollector *pool)
            {
                m_gcPools.push_back(pool);
            }

            void ResourceManager::clear(uint64_t currentFrame)
            {
                spdlog::info("Clear handles started");
                for (auto &pool : m_gcPools) {
                    pool->collectGarbage(currentFrame);
                }
                spdlog::info("Clear ended");
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
