#include "resource_manager.hpp"
#include "spdlog/spdlog.h"

#include "impl/profiler.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            ResourceManager::ResourceManager() {}

            ResourceManager &ResourceManager::GetInstance()
            {
                static ResourceManager instance;
                return instance;
            }

            void ResourceManager::registerPool(IPoolGarbageCollector *pool)
            {
                m_gcPools.push_back(pool);
            }

            void ResourceManager::clear()
            {
                if (m_currentTick % CLEAN_FRAME != 0) {
                    return;
                }
                ATMO_PROFILE_SCOPE_N_COLOR("Resource clear", 0xFF0000);
                for (auto &pool : m_gcPools) {
                    pool->collectGarbage(m_currentTick);
                }
            }
        } // namespace resource
    } // namespace core
} // namespace atmo
