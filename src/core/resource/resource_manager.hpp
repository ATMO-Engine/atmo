#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/resource/resource_key.hpp"
#include "core/resource/resource_loader_registry.hpp"

namespace atmo::core::resource
{
    class ResourceManager
    {
    public:
        static ResourceManager &GetInstance();

        ResourceManager(const ResourceManager &) = delete;
        ResourceManager &operator=(const ResourceManager &) = delete;

        template <typename T> std::shared_ptr<T> getResource(const std::string &path)
        {
            return getOrLoad<T>(path, path);
        }

    private:
        ResourceManager() = default;

        template <typename T> struct Store {
            std::mutex mutex;
            std::unordered_map<typename ResourceKey<T>::type, std::weak_ptr<T>, ResourceKeyHash<typename ResourceKey<T>::type>> entries;
        };

        template <typename T> static Store<T> &GetStore()
        {
            static Store<T> store;
            return store;
        }

        template <typename T> static std::shared_ptr<T> TryGet(Store<T> &store, const typename ResourceKey<T>::type &key)
        {
            std::lock_guard<std::mutex> lock(store.mutex);
            auto it = store.entries.find(key);
            return it != store.entries.end() ? it->second.lock() : nullptr;
        }

        template <typename T> static std::shared_ptr<T> LoadWithLoader(const std::string &path)
        {
            return ResourceLoaderRegistry::Instance().getOrCreateLoader<T>()->load(path);
        }

        template <typename T, typename... LoadArgs> std::shared_ptr<T> getOrLoad(const typename ResourceKey<T>::type &key, LoadArgs &&...loadArgs)
        {
            Store<T> &store = GetStore<T>();

            if (std::shared_ptr<T> existing = TryGet<T>(store, key)) {
                return existing;
            }

            std::shared_ptr<T> loaded = LoadWithLoader<T>(std::forward<LoadArgs>(loadArgs)...);

            std::lock_guard<std::mutex> lock(store.mutex);
            auto [it, inserted] = store.entries.try_emplace(key, loaded);
            if (!inserted) {
                if (std::shared_ptr<T> existing = it->second.lock()) {
                    return existing;
                }
                it->second = loaded;
            }
            return loaded;
        }
    };
} // namespace atmo::core::resource
