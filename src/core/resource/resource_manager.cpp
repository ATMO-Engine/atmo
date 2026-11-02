#include "resource_manager.hpp"

namespace atmo::core::resource
{
    ResourceManager &ResourceManager::GetInstance()
    {
        static ResourceManager instance;
        return instance;
    }
} // namespace atmo::core::resource
