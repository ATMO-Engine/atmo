#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <exception>
#include <stdexcept>

#include "resource_ref.hpp"
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
                }

                ~ResourcePool()
                {
                    for (uint32_t i = 0; i < m_entries.size(); ++i) {
                        auto& e = m_entries[i];
                        if (e.resource != nullptr) {
                            destroyEntry(i);
                        }
                    }
                }

                void collectGarbage(uint64_t currentFrame)
                {
                    constexpr uint64_t GRACE_FRAMES = 120;

                    for (uint32_t i = 0; i < m_entries.size(); ++i) {
                        auto& e = m_entries[i];
                        if (e.resource != nullptr) {
                            if (e.strongRefs == 0 && e.residentRefs == 0) {
                                if (currentFrame - e.lastUsedFrame > GRACE_FRAMES) {
                                    destroyEntry(i);
                                }
                            }
                        }
                    }
                }

                void retain(StoreHandle handle, uint64_t tick)
                {
                    m_entries[handle.index].lastUsedFrame = tick;
                    m_entries[handle.index].strongRefs++;
                }

                void release(StoreHandle handle)
                {
                    m_entries[handle.index].strongRefs--;
                }

                void pin(StoreHandle handle, uint64_t tick)
                {
                    m_entries[handle.index].lastUsedFrame = tick;
                    m_entries[handle.index].residentRefs++;
                }

                void unpin(StoreHandle handle, uint64_t tick)
                {
                    m_entries[handle.index].lastUsedFrame = tick;
                    m_entries[handle.index].residentRefs--;
                }

                const StoreHandle create(const std::string &path, uint64_t tick)
                {
                    try {
                        T *res = m_loader->load(path);
                        std::shared_ptr<T> ptr(res);

                        StoreHandle newHandle;
                        if (!m_freeList.empty()) {
                            std::uint16_t idx = m_freeList.back();
                            m_freeList.pop_back();
                            m_entries.at(idx).resource = ptr;
                            m_entries.at(idx).lastUsedFrame = tick;
                            m_entries.at(idx).strongRefs = 0;
                            m_entries.at(idx).residentRefs = 0;

                            newHandle.index = idx;
                            newHandle.generation = m_entries.at(idx).generation;
                        } else {
                            Entry newRes = {.resource = nullptr, .generation = 1,
                                            .strongRefs = 0, .residentRefs = 0,
                                            .lastUsedFrame = 0};
                            newRes.resource = ptr;
                            newRes.lastUsedFrame = tick;

                            newHandle.index = m_entries.size();
                            newHandle.generation = 1;

                            m_entries.push_back(newRes);
                        }

                        return newHandle;
                    } catch (const std::exception &e) {
                        throw e;
                    }
                }

                ResourceRef<T> getRef(StoreHandle &handle, uint64_t tick)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw std::runtime_error("Handle périmé");
                    }
                    ResourceRef<T> ref(this, handle, tick);
                    return ref;
                }

                std::shared_ptr<T> getAsset(const StoreHandle &handle)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw std::runtime_error("Handle périmé");
                    }
                    return m_entries.at(handle.index).resource;
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

                void destroyEntry(int index)
                {
                    if (m_entries[index].resource != nullptr) {
                        m_loader->destroy(m_entries[index].resource.get());// TODO: Implementer avec le système de caching (retirer la
                                                            // ressource du vecteur et l'envoyer dans le cache)
                        m_entries[index].generation += 1;
                        m_freeList.push_back(index);
                    }
                }

                std::unique_ptr<Resource<T>> m_loader;

                std::vector<Entry> m_entries;
                std::vector<std::uint16_t> m_freeList;
            };
        } // namespace resource
    } // namespace core
} // namespace atmo
