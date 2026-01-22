#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/resource/handle.hpp"
#include "core/resource/loader_dispatcher.hpp"
#include "handle.hpp"
#include "i_resource_pool.hpp"
#include "resource_pool.hpp"
#include "resource_ref.hpp"

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
                 * @param handle handle associated to the ressource you want to get
                 * @return std::any ressource ready to use
                 */
                template <typename T> ResourceRef<T> getResource(const std::string &path) // TODO: créer l'exception pour les handle périmé
                {
                    ResourceTypeStore<T> &store = getPool<T>();

                    if (store.mapHandle.find(path) != store.mapHandle.end()) {
                        try {
                            ResourceRef<T> ref = store.pool->getRef(store.mapHandle.at(path), m_currentTick);
                            return ref;
                        } catch (const ResourcePool<T>::HandleOutDated &e) {
                            StoreHandle newHandle = store.pool->create(path, m_currentTick);
                            store.mapHandle.at(path) = newHandle;

                            ResourceRef<T> ref = store.pool->getRef(store.mapHandle.at(path), m_currentTick);
                            return ref;
                        }
                    } else {
                        StoreHandle newHandle = store.pool->create(path, m_currentTick);
                        store.mapHandle.insert(std::pair<std::string, StoreHandle>(path, newHandle));

                        ResourceRef<T> ref = store.pool->getRef(newHandle, m_currentTick);
                        return ref;
                    }
                }


                /**
                 * @brief Clear unused handles
                 */
                void clear(uint64_t currentFrame);

                void increaseTick(uint64_t current)
                {
                    m_currentTick = current;
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

                uint64_t m_currentTick;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
