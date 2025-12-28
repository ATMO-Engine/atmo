#pragma once

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "core/resource/loader_dispatcher.hpp"
#include "handle.hpp"
#include "i_resource_pool.hpp"
#include "spdlog/spdlog.h"
#include "common/utils.hpp"
#include "core/resource/handle.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template<typename T>
            class ResourcePool;

            class ResourceManager
            {
            public:
                static ResourceManager &GetInstance();

                ~ResourceManager() = default;

                void registerPool(IPoolGarbageCollector* pool);

                /**
                 * @brief If needed generate the resource associated to the path given and give the Handle
                 *
                 * @param path absolute path of the resource you want to load
                 * @return Handle handle associated with the resource
                 */
                template<typename T>
                const Handle<T> generate(const std::string &path)
                {
                    std::string extension = atmo::common::Utils::splitString(path, '.').back();
                    try {
                        Handle<T> newHandle = getPool<T>().create(path);
                    } catch (const std::exception &e) {
                        spdlog::error(e.what());
                        throw e;
                    }
                }

                /**
                 * @brief get the resource associated to the handle if possible,
                 *        throw an exception if the handle is outdated
                 *
                 * @param handle handle associated to the ressource you want to get
                 * @return std::any ressource ready to use
                 */
                template<typename T>
                T getResource(const Handle<T> &handle) //TODO: créer l'exception pour les handle périmé
                {
                    std::string extension = atmo::common::Utils::splitString(handle->path, '.').back();
                    try {
                        getPool<T>().getFromHandle(handle);
                    } catch (const std::exception &e) {
                        spdlog::error(e.what());
                        throw e;
                    }
                }

                /**
                 * @brief Clear unused handles
                 */
                void clear(uint64_t currentFrame);

            private:
                ResourceManager();
                ResourceManager &operator=(const ResourceManager &) = delete;

                template<typename T>
                struct ResourceTypeStore {
                    ResourcePool<T> pool;
                    std::unordered_map<std::string, StoreHandle> mapHandle;
                    std::mutex mutex;
                };

                template<typename T>
                ResourceTypeStore<T>& getPool()
                {
                    static ResourceTypeStore<T> store = {.pool = ResourcePool<T>(createLoader<T>()),
                                                         .mapHandle = {},
                                                         .mutex = std::mutex()};
                    return store;
                }

                std::vector<IPoolGarbageCollector *> m_gcPools;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
