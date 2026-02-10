#pragma once

#include <cstdint>
#include <exception>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "spdlog/spdlog.h"

#include "core/resource/handle.hpp"
#include "core/resource/resource.hpp"
#include "i_resource_pool.hpp"
#include "resource_ref.hpp"
#include "impl/profiler.hpp"

namespace atmo
{
    namespace core
    {
        namespace resource
        {
            template <typename T> class ResourcePool : public IPoolGarbageCollector
            {
            public:
                class HandleOutDated : public std::exception
                {
                public:
                    HandleOutDated(const std::string &msg) : m_message(msg) {};
                    const char *what() const noexcept override
                    {
                        return m_message.c_str();
                    }

                private:
                    std::string m_message;
                };

                ResourcePool(std::unique_ptr<Resource<T>> loader) : m_loader(std::move(loader))
                {
                    m_entries = {};
                    m_freeList = {};
                }

                ~ResourcePool()
                {
                    for (uint32_t i = 0; i < m_entries.size(); ++i) {
                        auto &e = m_entries[i];
                        if (e.resource != nullptr) {
                            destroyEntry(i);
                        }
                    }
                }

                void collectGarbage(uint64_t currentFrame)
                {
                    ATMO_PROFILE_SCOPE_N("Pool");
                    ATMO_PROFILE_ZONE_NAME(m_loader->resourceTypeName().c_str(), m_loader->resourceTypeName().size());
                    ATMO_PROFILE_ZONE_TEXT("Current frame:", 15);
                    ATMO_PROFILE_ZONE_VALUE(currentFrame);

                    constexpr uint64_t GRACE_FRAMES = 120;

                    for (uint32_t i = 0; i < m_entries.size(); ++i) {
                        ATMO_PROFILE_SCOPE_N("Entry");
                        ATMO_PROFILE_ZONE_TEXT("Entry n:", 9);
                        ATMO_PROFILE_ZONE_VALUE(i);
                        auto &e = m_entries[i];
                        if (e.resource != nullptr) {
                            ATMO_PROFILE_ZONE_TEXT("Resident Ref:", 14);
                            ATMO_PROFILE_ZONE_VALUE(e.residentRefs);
                            ATMO_PROFILE_ZONE_TEXT("Strong Ref:", 12);
                            ATMO_PROFILE_ZONE_VALUE(e.strongRefs);
                            ATMO_PROFILE_ZONE_TEXT("Last Frame Use:", 16);
                            ATMO_PROFILE_ZONE_VALUE(e.lastUsedFrame);
                            if (e.strongRefs == 0 && e.residentRefs == 0) {
                                if (currentFrame - e.lastUsedFrame > GRACE_FRAMES) {
                                    ATMO_PROFILE_ZONE_TEXT("-- Resource delete --", 22);
                                    spdlog::debug("Destroy entry: " + std::to_string(i));
                                    destroyEntry(i);
                                }
                            }
                        } else {
                            ATMO_PROFILE_ZONE_TEXT("Entry empty", 12);
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

                void pin(StoreHandle handle)
                {
                    spdlog::debug("Pinned index: {}", handle.index);
                    m_entries[handle.index].residentRefs++;
                }

                void unpin(StoreHandle handle)
                {
                    spdlog::debug("Unpinned index: {}", handle.index);
                    m_entries[handle.index].residentRefs--;
                }

                const StoreHandle create(const std::string &path, uint64_t tick)
                {
                    ATMO_PROFILE_SCOPE_COLOR(0xFF0000);
                    std::shared_ptr<T> res = m_loader->load(path);

                    ATMO_PROFILE_ZONE_TEXT("Entry before creation:", 23);
                    ATMO_PROFILE_ZONE_VALUE(m_entries.size());
                    StoreHandle newHandle = {};
                    if (!m_freeList.empty()) {
                        ATMO_PROFILE_ZONE_TEXT("Recycled entry", 15);
                        std::uint16_t idx = m_freeList.back();
                        m_freeList.pop_back();
                        m_entries.at(idx).resource = res;
                        m_entries.at(idx).lastUsedFrame = tick;
                        m_entries.at(idx).strongRefs = 0;
                        m_entries.at(idx).residentRefs = 0;

                        newHandle.index = idx;
                        newHandle.generation = m_entries.at(idx).generation;
                    } else {
                        ATMO_PROFILE_ZONE_TEXT("Created new entry", 18);
                        Entry newRes = { .resource = nullptr, .generation = 1, .strongRefs = 0, .residentRefs = 0, .lastUsedFrame = 0 };
                        newRes.resource = res;
                        newRes.lastUsedFrame = tick;

                        newHandle.index = m_entries.size();
                        newHandle.generation = 1;

                        m_entries.push_back(newRes);
                    }

                    ATMO_PROFILE_ZONE_TEXT("Entry after creation:", 22);
                    ATMO_PROFILE_ZONE_VALUE(m_entries.size());

                    return newHandle;
                }

                ResourceRef<T> getRef(StoreHandle &handle, uint64_t tick)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw HandleOutDated("Handle " + std::to_string(handle.index) + " out dated");
                    }
                    ResourceRef<T> ref(this, handle, tick);
                    return ref;
                }

                std::shared_ptr<T> getAsset(const StoreHandle &handle)
                {
                    if (handle.generation != m_entries.at(handle.index).generation) {
                        throw HandleOutDated("Handle " + std::to_string(handle.index) + " out dated");
                    }
                    return m_entries.at(handle.index).resource;
                }

            private:
                struct Entry {
                    std::shared_ptr<T> resource;
                    uint32_t generation = 1;

                    uint32_t strongRefs = 0;
                    uint32_t residentRefs = 0;
                    uint32_t lastUsedFrame = 0;
                };

                void destroyEntry(int index)
                {
                    if (m_entries[index].resource != nullptr) {
                        m_entries[index].resource.reset(); // TODO: Implementer avec le système de caching (retirer la
                                                           // ressource du vecteur et l'envoyer dans le cache)
                        if (m_entries[index].resource != nullptr) {
                            throw std::runtime_error("Shared pointer has been stored outside the resource manager, so the resource couldn't be destroyed");
                        }
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
