#include "resource_manager.hpp"

namespace atmo::core::resource
{
    ResourceManager &ResourceManager::GetInstance()
    {
        static ResourceManager instance;
        return instance;
    }

    void ResourceManager::releaseRenderer(SDL_Renderer *renderer)
    {
        Store<SDL_Texture> &store = GetStore<SDL_Texture>();
        std::lock_guard<std::mutex> lock(store.mutex);
        std::erase_if(store.entries, [renderer](const auto &entry) { return entry.first.second == renderer; });
    }
} // namespace atmo::core::resource
