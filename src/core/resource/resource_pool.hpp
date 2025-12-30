#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <exception>
#include <stdexcept>

#include "resource_ref.hpp"
#include "resource_manager.hpp"
#include "core/resource/resource.hpp"
#include "core/resource/handle.hpp"
#include "i_resource_pool.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template<typename T>
            class ResourcePool : public IPoolGarbageCollector
            {
            public:
                ResourcePool(std::unique_ptr<Resource<T>> loader) : m_loader(std::move(loader))
                {
                    m_entries = {};
                    m_freeList = {};
                    ResourceManager::GetInstance().registerPool(this);
                }
                ~ResourcePool();

                void collectGarbage(uint64_t currentFrame)
                {
                    constexpr uint64_t GRACE_FRAMES = 120;

                    for (uint32_t i = 0; i < m_entries.size(); ++i) {
                        auto& e = m_entries[i];
                        if (e.resource != nullptr) {
                            if (e.strongRefs == 0 && e.residentRefs == 0) {
                                if (e.pendingDestroy && currentFrame - e.lastUsedFrame > GRACE_FRAMES) {
                                    destroyEntry(i);
                                }
                            }
                        }
                    }
                }

                void retain(StoreHandle handle)
                {
                    m_entries[handle.index].lastUsedFrame = ResourceManager::GetInstance().getTick();
                    m_entries[handle.index].strongRefs++;
                }

                void release(StoreHandle handle)
                {
                    m_entries[handle.index].lastUsedFrame = ResourceManager::GetInstance().getTick();
                    m_entries[handle.index].strongRefs--;
                }

                void pin(StoreHandle handle)
                {
                    m_entries[handle.index].lastUsedFrame = ResourceManager::GetInstance().getTick();
                    m_entries[handle.index].residentRefs++;
                }

                void unpin(StoreHandle handle)
                {
                    m_entries[handle.index].lastUsedFrame = ResourceManager::GetInstance().getTick();
                    m_entries[handle.index].residentRefs--;
                }

                const StoreHandle create(const std::string &path)
                {
                    try {
                        T res = m_loader->load(path);

                        StoreHandle newHandle;
                        if (!m_freeList.empty()) {
                            std::uint16_t idx = m_freeList.back();
                            m_freeList.pop_back();
                            m_entries.at(idx).resource = res;
                            m_entries.at(idx).lastUsedFrame = ResourceManager::GetInstance().getTick();
                            m_entries.at(idx).strongRefs = 0;
                            m_entries.at(idx).residentRefs = 0;

                            newHandle.index = idx;
                            newHandle.generation = m_entries.at(idx).generation;
                        } else {
                            Entry newRes = {.resource = nullptr, .generation = 1,
                                            .residentRefs = 0, .strongRefs = 0,
                                            .lastUsedFrame = 0};
                            newRes.resource = std::make_shared<T>(res);
                            newRes.lastUsedFrame = ResourceManager::GetInstance().getTick();

                            newHandle.index = m_entries.size();
                            newHandle.generation = 1;

                            m_entries.push_back(newRes);
                        }

                        return newHandle;
                    } catch (const std::exception &e) {
                        throw e;
                    }
                }

                ResourceRef<T> getRef(StoreHandle &handle)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw std::runtime_error("Handle périmé");
                    }
                    ResourceRef<T> ref(this, handle);
                    return ref;
                }

                T getAsset(const StoreHandle &handle)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw std::runtime_error("Handle périmé");
                    }
                    return m_entries.at(handle.index).resource;
                }

                void destroyEntry(int index)
                {
                    if (m_entries[index].resource != nullptr) {
                        m_entries[index].resource->destroy(); // TODO: Implementer avec le système de caching (retirer la
                                                            // ressource du vecteur et l'envoyer dans le cache)
                        m_entries[index].generation += 1;
                        m_freeList.push_back(index);
                    }
                }

                void destroy(const StoreHandle &handle)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw std::runtime_error("Handle périmé");
                    }
                    m_entries.at(handle.index).resource->destroy(); // TODO: Implementer avec le système de caching (retirer la
                                                            // ressource du vecteur et l'envoyer dans le cache)
                    m_entries.at(handle.index).generation += 1;
                    m_freeList.push_back(handle.index);
                }

            private:
                struct Entry
                {
                    std::shared_ptr<T> resource;
                    uint32_t generation = 1;

                    uint32_t strongRefs = 0;
                    uint32_t residentRefs = 0;
                    uint32_t lastUsedFrame = 0;
                };

                std::unique_ptr<Resource<T>> m_loader;

                std::vector<Entry> m_entries;
                std::vector<std::uint16_t> m_freeList;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
