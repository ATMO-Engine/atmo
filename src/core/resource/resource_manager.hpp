#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include "SDL3/SDL_render.h"

#include "core/resource/handle.hpp"
#include "core/resource/loader_dispatcher.hpp"
#include "i_resource_pool.hpp"
#include "resource_pool.hpp"
#include "resource_ref.hpp"

#define CLEAN_FRAME 60

namespace atmo
{
    namespace core
    {
        namespace resource
        {

            class ResourceManager
            {
            public:
                static ResourceManager &GetInstance();

                void setRenderer(SDL_Renderer *renderer)
                {
                    m_renderer = renderer;
                }
                SDL_Renderer *getRenderer() const
                {
                    return m_renderer;
                }

                ~ResourceManager()
                {
                    for (auto pool : m_gcPools) {
                        delete pool;
                    }
                }

                /**
                 * @brief get the resource associated to the handle if possible,
                 *        throw an exception if the handle is outdated
                 *
                 * @param path The path (project or absolute) of associated to the ressource you want to get
                 * @return std::unique_ptr<ResourceRef<T>> A unique ptr to the ResourceRef of the resource
                 */
                template <typename T> std::unique_ptr<ResourceRef<T>> getResource(const std::string &path) // TODO: créer l'exception pour les handle périmé
                {
                    ResourceTypeStore<T> &store = getPool<T>();

                    if (store.mapHandle.find(path) != store.mapHandle.end()) {
                        try {
                            return store.pool->getRef(store.mapHandle.at(path), m_currentTick);
                        } catch (const typename ResourcePool<T>::HandleOutDated &e) {
                            StoreHandle newHandle = store.pool->create(path, m_currentTick);
                            store.mapHandle.at(path) = newHandle;

                            return store.pool->getRef(store.mapHandle.at(path), m_currentTick);
                        }
                    } else {
                        StoreHandle newHandle = store.pool->create(path, m_currentTick);
                        store.mapHandle.insert(std::pair<std::string, StoreHandle>(path, newHandle));

                        return store.pool->getRef(newHandle, m_currentTick);
                    }
                }

                /**
                 * @brief Clear unused handles
                 */
                void clear();

                void increaseTick()
                {
                    m_currentTick++;
                }

                uint64_t getTick()
                {
                    return m_currentTick;
                }

            private:
                ResourceManager();
                ResourceManager &operator=(const ResourceManager &) = delete;

                template <typename T> struct ResourceTypeStore {
                    ResourcePool<T> *pool = nullptr;
                    std::unordered_map<std::string, StoreHandle> mapHandle;
                };

                /**
                 * @brief
                 * Get the Pool instance associated with the data type
                 *
                 * @tparam T The datatype
                 * @return ResourceTypeStore<T>& The pool
                 */
                template <typename T> ResourceTypeStore<T> &getPool()
                {
                    static ResourceTypeStore<T> store = { .pool = nullptr, .mapHandle = {} };
                    if (!store.pool) {
                        store.pool = new ResourcePool<T>(createLoader<T>());
                        registerPool(store.pool);
                    }
                    return store;
                }

                void registerPool(IPoolGarbageCollector *pool);

                std::vector<IPoolGarbageCollector *> m_gcPools;

                uint64_t m_currentTick = 0;
                SDL_Renderer *m_renderer = nullptr;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
